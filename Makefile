NAME :=			ircserv

# SOURCE FILES
SRCS_DIR :=		src
SRCS_FILES :=	main.cpp \
				Server.cpp \
				ServerUser.cpp \
				ServerSocket.cpp \
				ServerChannel.cpp \
				User.cpp \
				UserMessaging.cpp \
				UserRegistration.cpp \
				Command.cpp \
				CommandRegistration.cpp \
				CommandChannel.cpp \
				CommandModes.cpp \
				CommandMessaging.cpp \
				CommandConnection.cpp \
				CommandUtils.cpp \
				Channel.cpp \
				signal.cpp \
				utils.cpp

SRCS :=			$(addprefix $(SRCS_DIR)/, $(SRCS_FILES))
				
# OBJECT FILES
OBJS_DIR :=		obj
OBJS :=			$(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)
DEPS :=			$(OBJS:.o=.d)

# Detect the operating system
OS := $(shell uname -s)

# COMPILER
CXX :=			c++
CXXFLAGS :=		-std=c++98
CXXFLAGS +=		-Werror -Wextra -Wall
CXXFLAGS +=		-Wshadow	# Warns about shadowed variables.
CXXFLAGS +=		-Wpedantic	# Enforces strict ISO C++ compliance.
# CXXFLAGS +=		-g -O0

# CPPFLAGS are for preprocessor-specific flags
# Add OS-specific flags or definitions
ifeq ($(OS),Darwin) # Darwin is the kernel name for macOS
	# Define a preprocessor macro for macOS
	CPPFLAGS += -DMACOS_OS
else ifeq ($(OS),Linux)
	# Define a preprocessor macro for Linux
	CPPFLAGS += -DLINUX_OS
endif

# Used for progress bar
TOTAL_SRCS :=	$(words $(SRCS))
SRC_NUM :=		0

RESET =			\033[0m
BOLD =			\033[1m
GREEN =			\033[32m
YELLOW =		\033[33m
RED :=			\033[91m

###########
## RULES ##
###########

all:		$(NAME)

$(NAME):	$(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(BOLD)$(YELLOW)\n$(NAME) successfully compiled.$(RESET)"

## COMPILATION PROGRESS BAR ##
# Compiles individual .cpp files into .o object files without linking.
# Last line:
# -c:		Generates o. files without linking.
# -$<:		Represents the first prerequisite (the c. file).
# -o $@:	Output file name;  '$@' is replaced with target name (the o. file).
# -MMD:		Generates a dependency file for each source file.
# -MP:		Prevents make from failing if the header file is deleted.
$(OBJS_DIR)/%.o:	$(SRCS_DIR)/%.cpp
	@mkdir -p $(@D)
	@$(eval SRC_NUM := $(shell expr $(SRC_NUM) + 1))
	@$(eval PERCENT := $(shell printf "%.0f" $(shell echo "scale=4; $(SRC_NUM) / $(TOTAL_SRCS) * 100" | bc)))
	@printf "$(BOLD)\rCompiling $(NAME): ["
	@$(eval PROGRESS := $(shell expr $(PERCENT) / 5))
	@printf "$(GREEN)%0.s#$(RESET)$(BOLD)" $(shell seq 1 $(PROGRESS))
	@if [ $(PERCENT) -lt 100 ]; then printf "%0.s-" $(shell seq 1 $(shell expr 20 - $(PROGRESS))); fi
	@printf "] "
	@if [ $(PERCENT) -eq 100 ]; then printf "$(GREEN)"; fi
	@printf "%d/%d - " $(SRC_NUM) $(TOTAL_SRCS)
	@printf "%d%% $(RESET)" $(PERCENT)
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -MMD -MP $< -o $@

clean:
	@rm -rf $(OBJS_DIR)
	@echo "$(BOLD)$(RED)$(NAME) object files removed.$(RESET)"

fclean:	clean
	@rm -f $(NAME)
	@echo "$(BOLD)$(RED)$(NAME) removed.$(RESET)"

re:	fclean all

check_os:
	@echo "Detected OS: $(OS)"

.PHONY: all clean fclean re check_os

-include $(DEPS)