#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Reads a single character from standard input without waiting for the Enter key
 *        and without echoing it to the terminal.
 * 
 * This function temporarily disables canonical mode and echoing on the terminal
 * to allow for capturing a single key press immediately.
 * 
 * @return int (The character code for the key pressed)
 */
int getch()
{
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

/**
 * @brief Clean up a path string
 * 
 * Removes:
 *  - Trailing newline
 *  - Surrounding double quotes
 *  - Leading and trailing spaces
 * 
 * @param path (the path string to clean)
 */
void sanitize_path(char *path)
{
    if (!path) return;

    // Remove trailing newline
    path[strcspn(path, "\n")] = '\0';

    size_t len = strlen(path);
    if (len >= 2 && path[0] == '"' && path[len - 1] == '"')
    {
        memmove(path, path + 1, len - 2);
        path[len - 2] = '\0';
        len -= 2;
    }

    // Trim leading spaces
    char *start = path;
    while (*start == ' ') start++;

    // Trim trailing spaces
    char *end = path + strlen(path) - 1;
    while (end > start && *end == ' ') end--;
    *(end + 1) = '\0';

    // Move cleaned string to the beginning
    if (start != path)
        memmove(path, start, strlen(start) + 1);
}