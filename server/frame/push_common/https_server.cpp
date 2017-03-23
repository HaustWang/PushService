
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/ec.h>

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#include "https_server.h"
#include "log4cpp_log.h"


#define DEFAULT_HTTPS_PORT 8421
#define CA_CERT "ca.cert"
#define SERVER_CERT "server.cert"
#define SERVER_KEY  "server.key"

typedef union
{
    struct sockaddr_storage ss;
    struct sockaddr sa;
    struct sockaddr_in in;
    struct sockaddr_in6 i6;
} sock_hop;

extern struct event_base *g_event_base;

static struct bufferevent* bevcb (struct event_base *base, void *arg)
{
    SSL_CTX *ctx = (SSL_CTX *) arg;

    struct bufferevent* r = bufferevent_openssl_socket_new (base,
                                      -1,
                                      SSL_new (ctx),
                                      BUFFEREVENT_SSL_ACCEPTING,
                                      BEV_OPT_CLOSE_ON_FREE);
    return r;
}

static void *my_zeroing_malloc (size_t howmuch)
{
    return calloc (1, howmuch);
}

void HttpsServer::InitEnv(unsigned short port, handle http_handle)
{
    CRYPTO_set_mem_functions (my_zeroing_malloc, realloc, free);
    SSL_library_init ();
    SSL_load_error_strings ();
    OpenSSL_add_all_algorithms ();

    log_info ("Using OpenSSL version \"%s\"\nand libevent version \"%s\"\n",
          SSLeay_version (SSLEAY_VERSION),
          event_get_version ());

    m_listen_port = port;
    m_http_handle = http_handle;
}

int HttpsServer::StartHttpsService (void)
{
    if(NULL == g_event_base)
    {
        log_error("StartHttpsService failed cause g_event_base is NULL");
        return -1;
    }

    struct evhttp *http = evhttp_new (g_event_base);
    if (NULL == http)
    {
        log_error ("couldn't create evhttp. Exiting.\n");
        return 1;
    }

    SSL_CTX *ctx = SSL_CTX_new (SSLv23_server_method ());
    SSL_CTX_set_options (ctx,
                       SSL_OP_SINGLE_DH_USE |
                       SSL_OP_SINGLE_ECDH_USE |
                       SSL_OP_NO_SSLv2);

    EC_KEY *ecdh = EC_KEY_new_by_curve_name (NID_X9_62_prime256v1);
    if (NULL == ecdh)
    {
        log_error("EC_KEY_new_by_curve_name failed");
        return -1;
    }

    if (1 != SSL_CTX_set_tmp_ecdh (ctx, ecdh))
    {
        log_error ("SSL_CTX_set_tmp_ecdh failed");
        return -1;
    }

    if(0 != LoadCertFile(ctx))
        return -1;

    evhttp_set_bevcb (http, bevcb, ctx);

    evhttp_set_gencb (http, m_http_handle, NULL);

    struct evhttp_bound_socket* handle = evhttp_bind_socket_with_handle (http, "0.0.0.0", m_listen_port);
    if (NULL == handle)
    {
        log_error("couldn't bind to port %d. Exiting.\n",
               (int) m_listen_port);
        return -1;
    }

    sock_hop ss;
    evutil_socket_t fd;
    ev_socklen_t socklen = sizeof (ss);
    char addrbuf[128];
    void *inaddr;
    const char *addr;
    int got_port = -1;
    fd = evhttp_bound_socket_get_fd (handle);
    memset (&ss, 0, sizeof(ss));
    if (getsockname (fd, &ss.sa, &socklen))
    {
        log_error ("getsockname() failed, err:%s", strerror(errno));
        return -1;
    }

    if (ss.ss.ss_family == AF_INET)
    {
        got_port = ntohs (ss.in.sin_port);
        inaddr = &ss.in.sin_addr;
    }
    else if (ss.ss.ss_family == AF_INET6)
    {
        got_port = ntohs (ss.i6.sin6_port);
        inaddr = &ss.i6.sin6_addr;
    }
    else
    {
        log_error("Weird address family %d\n", ss.ss.ss_family);
        return -1;
    }

    addr = evutil_inet_ntop (ss.ss.ss_family, inaddr, addrbuf,
                             sizeof (addrbuf));
    if (NULL == addr)
        log_info ("https Listening on %s:%d\n", addr, got_port);
    else
    {
        log_error("evutil_inet_ntop failed\n");
        return 1;
    }

  return 0;
}

int HttpsServer::LoadCertFile(SSL_CTX *ctx)
{
    log_info ("Loading verify chain from '%s', certificate chain from '%s' and private key from '%s'\n",
               CA_CERT, SERVER_CERT, SERVER_KEY);

    if(1 != SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL))
    {
        log_error("SSL_CTX_load_verify_locations load verify chain failed");
        return -1;
    }

    if (1 != SSL_CTX_use_certificate_chain_file (ctx, SERVER_CERT))
    {
        log_error("SSL_CTX_use_certificate_chain_file loading certificate chain failed!");
        return -1;
    }


    if (1 != SSL_CTX_use_PrivateKey_file (ctx, SERVER_KEY, SSL_FILETYPE_PEM))
    {
        log_error("SSL_CTX_use_PrivateKey_file load private_key failed!");
        return -1;
    }

    if (1 != SSL_CTX_check_private_key (ctx))
    {
        log_error("SSL_CTX_check_private_key check private key failed!");
        return -1;
    }

    return 0;
}



