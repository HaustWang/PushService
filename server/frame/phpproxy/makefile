BINARY := phpproxy 
PROJECT_DIR := ../..

.PHONY: all clean
all: $(BINARY)

-include $(PROJECT_DIR)/makefile.shared

LD_FLAGS += -BStatic -lhiredis -rdynamic

SRCS = $(wildcard *.cpp *.c *.cc *.cxx)
OBJ = $(patsubst %.cpp, $(DEP_DIR)/%.o, $(patsubst %.c ,$(DEP_DIR)/%.o, $(patsubst %.cc, $(DEP_DIR)/%.o, $(patsubst %.cxx, $(DEP_DIR)/%.o, $(SRCS) ) ) ) )

$(BINARY) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o $@ $^ $(LD_FLAGS)

# Clean project
cleantarget:
	rm -rf $(BINARY)

install:
	cp -f $(BINARY) ./bin

clean:
	rm -rf $(BINARY) $(DEP_DIR) *.d.* *.d *.o &2>/dev/null

-include $(PROJECT_DIR)/makefile.rule
