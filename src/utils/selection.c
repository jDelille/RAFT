#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./defs.h"
#include "./utils.h"

int selection(const char *prompt, const char *options[], int num_options)
{
    int selected = 0;
    int ch;

    int is_yes_no = (num_options == 2 &&
                     strcmp(options[0], "Yes") == 0 &&
                     strcmp(options[1], "No") == 0);

    printf("%s %s ", ICON_QUESTION, prompt);
    if (!is_yes_no)
    {
        printf("%s- Use arrow keys. Return to submit.%s\n", STYLE_GRAY, STYLE_RESET);
    }
    fflush(stdout);

    HIDE_CURSOR();

    while (1)
    {
        if (is_yes_no)
        {
            // Redraw horizontal Yes/No menu
            printf("\r\033[2K%s %s » ", ICON_QUESTION, prompt);
            for (int i = 0; i < num_options; i++)
            {
                if (i == selected)
                    printf("%s%s%s", STYLE_BLUE_UNDERLINE, options[i], STYLE_RESET);
                else
                    printf("%s", options[i]);
                if (i < num_options - 1)
                    printf(" / ");
            }
            fflush(stdout);

            ch = getch();
            if (ch == KEY_ESC && getch() == KEY_BRACKET)
            {
                int arrow = getch();
                if (arrow == KEY_ARROW_RIGHT && selected < num_options - 1)
                    selected++;
                if (arrow == KEY_ARROW_LEFT && selected > 0)
                    selected--;
            }
            else if (ch == KEY_ENTER || ch == '\n' || ch == '\r')
            {
                SHOW_CURSOR();
                printf("\r\033[2K%s %s » %s\n", GREEN_CHECK, prompt, options[selected]);
                return selected;
            }
        }
        else
        {
            // Original vertical list logic
            for (int i = 0; i < num_options; i++)
            {
                if (i == selected)
                    PRINT_HIGHLIGHTED(options[i]);
                else
                    PRINT_NORMAL(options[i]);
            }

            ch = getch();

            if (ch == KEY_ESC && getch() == KEY_BRACKET)
            {
                int arrow = getch();
                if (arrow == KEY_ARROW_DOWN && selected < num_options - 1)
                    selected++;
                if (arrow == KEY_ARROW_UP && selected > 0)
                    selected--;
            }
            else if (ch == KEY_ENTER || ch == '\n' || ch == '\r')
            {
                SHOW_CURSOR();
                // Clear previous menu (prompt + list)
                MOVE_CURSOR_UP(num_options + 1);
                for (int i = 0; i < num_options + 1; i++)
                {
                    CLEAR_LINE();
                    if (i < num_options) MOVE_CURSOR_DOWN(1);
                }
                MOVE_CURSOR_UP(num_options);

                printf("%s %s » %s\n", GREEN_CHECK, prompt, options[selected]);
                return selected;
            }

            // Redraw vertical menu
            MOVE_CURSOR_UP(num_options);
            for (int i = 0; i < num_options; i++)
            {
                CLEAR_TO_EOL_PRINTLN();
            }
            MOVE_CURSOR_UP(num_options);
        }
    }
}