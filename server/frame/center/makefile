BINARY := center
PROJECT_DIR := ../..

.PHONY: all clean cleantarget install
all: $(BINARY)

-include $(PROJECT_DIR)/makefile.shared

SRCS = $(wildcard *.cpp *.c *.cc *.cxx)
OBJ = $(patsubst %.cpp, $(DEP_DIR)/%.o, $(patsubst %.c ,$(DEP_DIR)/%.o, $(patsubst %.cc, $(DEP_DIR)/%.o, $(patsubst %.cxx, $(DEP_DIR)/%.o, $(SRCS) ) ) ) )

$(BINARY) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o $@ $^ $(LD_FLAGS)

# Clean project
cleantarget:
	rm -rf $(BINARY)

clean:
	rm -rf $(BINARY) $(DEP_DIR) *.d.* *.d *.o &2>/dev/null

install:
	cp -f $(BINARY) $(BIN_DIR)/
	cp config.json $(BIN_DIR)/

-include $(PROJECT_DIR)/makefile.rule
