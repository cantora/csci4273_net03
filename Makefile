.SECONDARY:

#DEFINES 		= -DNET03_DEBUG_LOG
DEFINES			+= -DNET03_ON_MSG_CALLBACK
DEFINES			+= -DNET03_SLOW_DOWN_UDP_SEND_RATE
#DEFINES			+= -DNET02_DEBUG_LOG 
INCLUDES 		+= -iquote"./src" -iquote"./net02" -iquote"./net01" 
#DBG				= -g
OPTIMIZE		= -O3
C_FLAGS 		= -Wall -Wextra $(OPTIMIZE) $(DBG) -w $(DEFINES) $(INCLUDES) -pthread

CXX_FLAGS		= $(C_FLAGS)
CXX_CMD			= g++ $(CXX_FLAGS)


BUILD 			= ./build

OBJECTS 		:= $(patsubst %.cc, $(BUILD)/%.o, $(notdir $(wildcard ./src/*.cc) ) ) $(patsubst %.cc, $(BUILD)/%.o, $(notdir $(wildcard ./net02/*.cc) ) ) $(patsubst %.cc, $(BUILD)/%.o, $(notdir $(wildcard ./net01/*.cc) ) )

DEPENDS			:= $(OBJECTS:.o=.d) 
DEPSDIR			= $(BUILD)
DEP_FLAGS		= -MMD -MP -MF $(patsubst %.o, %.d, $@)

TESTS 			= $(notdir $(patsubst %.cc, %, $(wildcard ./test/*.cc) ) )
TEST_OUTPUTS	= $(foreach test, $(TESTS), $(BUILD)/test/$(test))

default: all

.PHONY: all
all: $(OBJECTS)

test_arch: $(BUILD)/test_arch
	$(BUILD)/test_arch $(ARG)

$(BUILD)/test_arch: test_arch.cc $(OBJECTS)
	$(CXX_CMD) $+ -o $@

$(BUILD)/%.o: src/%.cc src/%.h ./Makefile
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(BUILD)/%.o: net02/%.cc net02/%.h ./Makefile
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(BUILD)/%.o: net01/%.cc net01/%.h ./Makefile
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(BUILD)/test/%.o: test/%.cc $(OBJECTS)
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(BUILD)/test/%: $(BUILD)/test/%.o $(OBJECTS)
	$(CXX_CMD) $+ $(LIB) -o $@

define test-template
$(1): $$(BUILD)/test/$(1) 
	$(BUILD)/test/$(1)
endef

.PHONY: $(TESTS) 
$(foreach test, $(TESTS), $(eval $(call test-template,$(test)) ) )

.PHONY: clean 
clean: 
	rm -vf $(shell find $(BUILD) -type f -not -name .gitignore )

-include $(DEPENDS)