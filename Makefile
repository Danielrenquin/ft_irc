# **************************************************************************** #
#                                 VARIABLES                                    #
# **************************************************************************** #

NAME		= ircserv

CXX			= c++
CXXFLAGS	= -std=c++98 #-Wall -Wextra -Werror 
RM			= rm -rf

# **************************************************************************** #
#                                 DIRECTORIES                                  #
# **************************************************************************** #

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= include

# **************************************************************************** #
#                                 SOURCE FILES                                 #
# **************************************************************************** #

SRCS		= $(SRC_DIR)/main.cpp \
			  $(SRC_DIR)/Server.cpp \
			  $(SRC_DIR)/Client.cpp \
			  $(SRC_DIR)/Channel.cpp \
			  $(SRC_DIR)/ErrorHandler.cpp \
			  $(SRC_DIR)/CommandHandler.cpp \
			  $(SRC_DIR)/commands/auth.cpp \
			  $(SRC_DIR)/commands/channel.cpp \
			  $(SRC_DIR)/commands/message.cpp \
			  $(SRC_DIR)/commands/operator.cpp \
			  $(SRC_DIR)/utils/parsing.cpp \
			  $(SRC_DIR)/utils/validation.cpp \

# **************************************************************************** #
#                                 OBJECT FILES                                 #
# **************************************************************************** #

OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# **************************************************************************** #
#                                 HEADER FILES                                 #
# **************************************************************************** #

INCS		= -I$(INC_DIR)

HEADERS		= $(INC_DIR)/Server.hpp \
			  $(INC_DIR)/Client.hpp \
			  $(INC_DIR)/Channel.hpp \
			  $(INC_DIR)/CommandHandler.hpp \
			  $(INC_DIR)/ErrorHandler.hpp \
			  $(INC_DIR)/utils.hpp

# **************************************************************************** #
#                                   COLORS                                     #
# **************************************************************************** #

GREEN		= \033[0;32m
RED			= \033[0;31m
BLUE		= \033[0;34m
YELLOW		= \033[0;33m
RESET		= \033[0m

# **************************************************************************** #
#                                    RULES                                     #
# **************************************************************************** #

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(BLUE)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@$(RM) $(OBJ_DIR)
	@echo "$(GREEN)✓ Clean complete!$(RESET)"

fclean: clean
	@echo "$(RED)Removing $(NAME)...$(RESET)"
	@$(RM) $(NAME)
	@echo "$(GREEN)✓ Full clean complete!$(RESET)"

re: fclean all

# **************************************************************************** #
#                                   BONUS                                      #
# **************************************************************************** #

BONUS_SRCS	= $(SRC_DIR)/bonus/FileTransfer.cpp \
			  $(SRC_DIR)/bonus/Bot.cpp

BONUS_OBJS	= $(BONUS_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

bonus: $(NAME) $(BONUS_OBJS)
	@echo "$(BLUE)Linking with bonus...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) $(BONUS_OBJS) -o $(NAME)
	@echo "$(GREEN)✓ Bonus compiled successfully!$(RESET)"

# **************************************************************************** #
#                                   PHONY                                      #
# **************************************************************************** #

.PHONY: all clean fclean re bonus
