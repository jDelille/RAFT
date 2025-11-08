// cli_defs.h
#ifndef CLI_DEFS_H
#define CLI_DEFS_H

// Styling
#define STYLE_CYAN_BLUE        "\033[1;36m"
#define STYLE_BLUE_UNDERLINE "\033[1;36m"
#define STYLE_RESET          "\033[0m"
#define HIGHLIGHT_TEMPLATE     STYLE_CYAN_BLUE
#define ARROW_COLOR            STYLE_CYAN_BLUE
#define STYLE_GRAY "\033[90m"
#define STYLE_GREEN "\033[1;32m"

#define PRINT_HIGHLIGHTED(text) \
    printf("  %s> %s%s%s\n", ARROW_COLOR, HIGHLIGHT_TEMPLATE, text, STYLE_RESET)
#define PRINT_NORMAL(text)      printf("    %s\n", text)

// Cursor Movement
#define MOVE_CURSOR_UP(lines)    printf("\033[%dA", lines)
#define MOVE_CURSOR_DOWN(lines)  printf("\033[%dB", lines)
#define CLEAR_LINE()             printf("\033[2K")
#define CLEAR_TO_EOL()           printf("\033[K")
#define CLEAR_TO_EOL_PRINTLN()   do { printf("\033[K"); putchar('\n'); } while (0)
#define HIDE_CURSOR()            printf("\033[?25l")
#define SHOW_CURSOR()            printf("\033[?25h")

#define CLEAR_PREV_LINE()           do { printf("\033[1A"); printf("\033[2K"); } while (0)
#define CLEAR_NEXT_LINE()           do { printf("\033[1B"); printf("\033[2K"); } while (0)

// Keyboard
#define KEY_ESC          27
#define KEY_BRACKET      91
#define KEY_ARROW_RIGHT  67
#define KEY_ARROW_LEFT   68
#define KEY_ARROW_UP     65
#define KEY_ARROW_DOWN   66
#define KEY_ENTER        10

// Icons
#define ICON_QUESTION "\033[34m?\033[0m"
#define GREEN_CHECK   "\033[1;32m✔\033[0m"

#endif // CLI_DEFS_H