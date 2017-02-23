BINARY := libjson.a
SRCDIR := .
.PHONY: all clean
all: cleantarget $(BINARY)

$(shell if [ ! -d .obj ]; then mkdir -p .obj; fi )

# Analyze project
CPP_FILES := $(shell find $(SRCDIR) -name "*.cpp")
INC_DIR   := $(addprefix -I,$(shell find $(SRCDIR) -type d | egrep -v '\.\.$$|\.svn'))
OBJ_FILES := $(subst /,-,$(subst $(SRCDIR)/,,$(CPP_FILES:%.cpp=%.o)))
OBJ_FILES := $(addprefix .obj/,$(OBJ_FILES))
DEP_FILES := $(subst /,-,$(subst $(SRCDIR)/,,$(CPP_FILES:%.cpp=%.d)))
DEP_FILES := $(addprefix .obj/,$(DEP_FILES))

CPPFLAGS = -g -Wall  -Werror -fPIC $(INC_DIR)

ARFLAGS = -rc

# Generate dependence, including rules to generate object files
.obj/%.d:
	@echo Generate $@ ...
	@g++ -MM $(CPPFLAGS) $(SRCDIR)/$(subst -,/,$*).cpp > $@.$$$$; \
	sed 's,.*:,.obj/$*.o:,g' < $@.$$$$ >$@; \
	echo '	g++ -c $(CPPFLAGS) $(SRCDIR)/$(subst -,/,$*).cpp -o .obj/$*.o' >>$@; \
	echo '$@: $(SRCDIR)/$(subst -,/,$*).cpp' >> $@; \
	rm -f $@.$$$$;

# Include dependence
ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

ifneq ($(BINARY),)
$(BINARY): $(OBJ_FILES)
	ar -cvr $@ $^
endif

# Clean project
cleantarget:
	-rm -rf $(BINARY)

install:
	cp -f $(BINARY) ../libs/

clean:
	-rm -rf $(BINARY) $(OBJ_FILES) $(DEP_FILES) *.d.* *.d *.o &2>/dev/null

