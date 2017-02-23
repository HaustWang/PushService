
#ifndef __COMMON_SINGLETON_H__
#define __COMMON_SINGLETON_H__

template<typename T> //, class CreatePolicy = OperatorNew<T> >
class  Singleton
{
public:
    static T &Instance()
    {
        if (NULL == si_instance)
        {
            si_instance = new T;
        }

        return *si_instance;
    }

protected:
    Singleton() {};

private:

    // Prohibited actions...this does not prevent hijacking.
    Singleton(const Singleton &);
    Singleton &operator=(const Singleton &);

    // Singleton Helpers
    //static void DestroySingleton();

    static T *si_instance;
    //static bool si_destroyed;
};

template<typename T>
T *Singleton<T>::si_instance = NULL;

#endif

