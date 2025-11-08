#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>

#include "../utils/defs.h"

bool file_exists(const char *file_path)
{
    return access(file_path, F_OK) == 0;
}

void convert_windows_path_to_wsl(char *file_path)
{

    bool is_windows_path =
        strlen(file_path) >= 3 &&
        file_path[1] == ':' &&
        (file_path[2] == '\\' || file_path[2] == '/');
    ;

    if (is_windows_path)
    {
        char wsl_path[512];
        snprintf(wsl_path, sizeof(wsl_path), "/mnt/%c/%s", tolower(file_path[0]), file_path + 3);
        strncpy(file_path, wsl_path, 512);
    }

    for (char *current_char = file_path; *current_char; ++current_char)
    {
        if (*current_char == '\\')
        {
            *current_char = '/';
        }
    }
}

bool ensure_template_dir_exists(void)
{
    return access(".templates", F_OK) == 0 || mkdir(".templates", 0700) == 0;
}

char *get_filename(const char *path)
{
    char *path_copy = strdup(path);
    char *base_name = basename(path_copy);
    char *result = strdup(base_name);
    free(path_copy);
    return result;
}

/* Copy a file from src to dst */
bool copy_file(const char *src_path, const char *dst_path)
{

    FILE *src_file = fopen(src_path, "r");
    if (!src_file)
    {
        perror("Error opening source");
        return false;
    }

    FILE *dst_file = fopen(dst_path, "w");
    if (!dst_file)
    {
        perror("Error creating dest");
        fclose(src_file);
        return false;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0)
    {
        fwrite(buffer, 1, bytes_read, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
    return true;
}

void install_template(const char *template_path)
{
    // Copy path and convert path to wsl
    char converted_path[512];
    snprintf(converted_path, sizeof(converted_path), "%s", template_path);
    convert_windows_path_to_wsl(converted_path);

    if (!ensure_template_dir_exists())
    {
        return;
    }

    if (!file_exists(converted_path))
    {
        printf("Error: The file %s does not exist\n", converted_path);
        return;
    }

    char *filename = get_filename(converted_path);

    // Build destination path inside .templates folder
    char destination_path[512];
    snprintf(destination_path, sizeof(destination_path), ".templates/%s", filename);
    free(filename);

    if (file_exists(destination_path))
    {
        printf("Template already exists: %s\n", destination_path);
        return;
    }
    if (copy_file(converted_path, destination_path))
        printf("Installed template: %s\n", destination_path);
}

const char *get_placeholder(
    const char *placeholder_name,
    const char *placeholder_keys[],
    const char *placeholder_values[],
    int num_placeholders)
{
    for (int i = 0; i < num_placeholders; i++)
        if (strcmp(placeholder_name, placeholder_keys[i]) == 0)
            return placeholder_values[i];
    return "";
}

void create_dir_from_line(const char *line, const char *projectName)
{
    // Skip leading slashes and spaces
    const char *dir_name = line;
    while (*dir_name == '/' || *dir_name == ' ')
        dir_name++;

    // Skip empty lines
    if (*dir_name == '\0')
        return;

    size_t dir_length = strlen(dir_name);

    // Not a directory, skip
    if (dir_name[dir_length - 1] != '/')
    {
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%.*s", projectName, (int)(dir_length - 1), dir_name); // remove trailing /

    mkdir(path, 0755);
}

static void trim_trailing_whitespace(char *string)
{
    size_t length = strlen(string);
    while (length > 0 && isspace((unsigned char)string[length - 1]))
    {
        string[--length] = '\0';
    }
}

static void ensure_parent_dirs(const char *file_path)
{
    char tmp[512];
    strcpy(tmp, file_path);
    char *dir = dirname(tmp);
    if (dir)
    {
        mkdir(dir, 0755);
    }
}

static void replace_placeholders(
    const char *input_line,
    const char **placeholder_keys,
    const char **replacement_values,
    int key_count,
    char *output,
    size_t output_size)
{
    output[0] = '\0';
    const char *cursor = input_line;

    while (*cursor)
    {
        const char *start_bracket = strchr(cursor, '[');
        if (!start_bracket)
        {
            strncat(output, cursor, output_size - strlen(output) - 1);
            break;
        }

        strncat(output, cursor, start_bracket - cursor);

        const char *end_bracket = strchr(start_bracket, ']');
        if (!end_bracket)
        {
            strncat(output, start_bracket, output_size - strlen(output) - 1);
            break;
        }

        char placeholder[128];
        char default_value[128] = "";

        size_t key_len = end_bracket - start_bracket - 1;
        if (key_len >= sizeof(placeholder))
            key_len = sizeof(placeholder) - 1;
        strncpy(placeholder, start_bracket + 1, key_len);
        placeholder[key_len] = '\0';

        char *equal_sign = strchr(placeholder, '=');

        if (equal_sign)
        {
            *equal_sign = '\0';
            strncpy(default_value, equal_sign + 1, sizeof(default_value));
        }

        // Find replacement from user input
        const char *replacement = default_value;

        for (int i = 0; i < key_count; i++)
        {
            if (strcmp(placeholder, placeholder_keys[i]) == 0)
            {
                replacement = replacement_values[i];
                break;
            }
        }

        strncat(output, replacement, output_size - strlen(output) - 1);
        cursor = end_bracket + 1;
    }
}

void create_file_from_line(
    FILE *template_fp,
    char *output_path,
    const char **placeholder_keys,
    const char **replacement_values,
    int key_count)
{
    trim_trailing_whitespace(output_path);
    ensure_parent_dirs(output_path);

    FILE *output_fp = fopen(output_path, "w");

    if (!output_fp)
    {
        perror("File create failed");
        return;
    }

    char input_line[1024];
    char processed_line[4096];

    while (fgets(input_line, sizeof(input_line), template_fp))
    {
        if (strncmp(input_line, "// ", 3) == 0)
        {
            fseek(template_fp, -strlen(input_line), SEEK_CUR); // Backtrack for next file
            break;
        }

        replace_placeholders(
            input_line,
            placeholder_keys,
            replacement_values,
            key_count,
            processed_line,
            sizeof(processed_line));

        fputs(processed_line, output_fp);
    }

    fclose(output_fp);
}

int extract_placeholders_from_template(
    const char *template_name,
    char placeholder_keys[][128],
    char placeholder_defaults[][128],
    int max_placeholders)
{
    char template_path[512];
    snprintf(template_path, sizeof(template_path), ".templates/%s", template_name);

    FILE *file = fopen(template_path, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Couldn't open template file %s\n", template_path);
        return 0;
    }

    char line[1024];
    int placeholder_count = 0;

    while (fgets(line, sizeof(line), file))
    {
        const char *cursor = line;

        while ((cursor = strchr(cursor, '[')) != NULL)
        {
            const char *end_bracket = strchr(cursor, ']');
            if (!end_bracket)
                break;

            char content[256];
            size_t content_len = end_bracket - cursor - 1;
            if (content_len >= sizeof(content))
                content_len = sizeof(content) - 1;

            strncpy(content, cursor + 1, content_len);
            content[content_len] = '\0';

            // Split into key and default value
            char *equal_sign = strchr(content, '=');
            char key[128] = "";
            char default_value[128] = "";

            if (equal_sign)
            {
                *equal_sign = '\0';
                strncpy(key, content, sizeof(key));
                strncpy(default_value, equal_sign + 1, sizeof(default_value));
            }
            else
            {
                strncpy(key, content, sizeof(key));
                default_value[0] = '\0';
            }

            // Trim whitespace
            key[strcspn(key, " \r\n\t")] = 0;
            default_value[strcspn(default_value, " \r\n\t")] = 0;

            // Skip empty keys
            if (strlen(key) == 0)
            {
                cursor = end_bracket + 1;
                continue;
            }

            // Avoid duplicates
            bool already_exists = false;
            for (int i = 0; i < placeholder_count; i++)
            {
                if (strcmp(placeholder_keys[i], key) == 0)
                {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists && placeholder_count < max_placeholders)
            {
                strncpy(placeholder_keys[placeholder_count], key, sizeof(placeholder_keys[0]));
                strncpy(placeholder_defaults[placeholder_count], default_value, sizeof(placeholder_defaults[0]));
                // printf("DEBUG: Found placeholder '%s' with default '%s'\n", key, default_value);
                placeholder_count++;
            }

            cursor = end_bracket + 1;
        }
    }

    fclose(file);
    return placeholder_count;
}

/* Generate a new file from a template */
void generate_project_from_template(
    const char *templateName,
    const char *projectName)
{
    char templatePath[512];
    snprintf(templatePath, sizeof(templatePath), ".templates/%s", templateName);

    FILE *templateFile = fopen(templatePath, "r");
    if (!templateFile)
    {
        printf("Template open failed: %s\n", templatePath);
        return;
    }

    mkdir(projectName, 0755);

    char line[1024];

    while (fgets(line, sizeof(line), templateFile))
    {
        // Ignore comment headers
        if (strncmp(line, "// #", 4) == 0)
            continue;

        // Handle files or directories
        if (strncmp(line, "//", 2) == 0)
        {
            size_t len = strlen(line);

            // Directory (trailing slash)
            if (line[len - 2] == '/')
            {
                create_dir_from_line(line + 2, projectName);
                continue;
            }

            // File
            char currentFile[512];
            snprintf(currentFile, sizeof(currentFile), "%s/%s", projectName, line + 3);
            currentFile[strcspn(currentFile, "\n")] = 0;

            // --- Extract placeholders from this file section ---
            #define MAX_PLACEHOLDERS 20
            char placeholder_keys[MAX_PLACEHOLDERS][128];
            char placeholder_defaults[MAX_PLACEHOLDERS][128];
            char user_values[MAX_PLACEHOLDERS][128];

            int num_placeholders = 0;

            long file_pos = ftell(templateFile); // Remember where file section starts

            char file_line[1024];
            while (fgets(file_line, sizeof(file_line), templateFile))
            {
                // Stop at next file or directory
                if (strncmp(file_line, "//", 2) == 0)
                {
                    fseek(templateFile, -strlen(file_line), SEEK_CUR);
                    break;
                }

                const char *cursor = file_line;
                while ((cursor = strchr(cursor, '[')) != NULL)
                {
                    const char *end_bracket = strchr(cursor, ']');
                    if (!end_bracket) break;

                    char content[256];
                    size_t content_len = end_bracket - cursor - 1;
                    if (content_len >= sizeof(content)) content_len = sizeof(content)-1;
                    strncpy(content, cursor + 1, content_len);
                    content[content_len] = '\0';

                    char *equal_sign = strchr(content, '=');
                    char key[128] = "";
                    char default_value[128] = "";

                    if (equal_sign)
                    {
                        *equal_sign = '\0';
                        strncpy(key, content, sizeof(key));
                        strncpy(default_value, equal_sign + 1, sizeof(default_value));
                    }
                    else
                    {
                        strncpy(key, content, sizeof(key));
                        default_value[0] = '\0';
                    }

                    key[strcspn(key, " \r\n\t")] = 0;
                    default_value[strcspn(default_value, " \r\n\t")] = 0;

                    // Skip empty keys
                    if (strlen(key) == 0)
                    {
                        cursor = end_bracket + 1;
                        continue;
                    }

                    // Avoid duplicates
                    bool exists = false;
                    for (int i = 0; i < num_placeholders; i++)
                    {
                        if (strcmp(placeholder_keys[i], key) == 0)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists && num_placeholders < MAX_PLACEHOLDERS)
                    {
                        strncpy(placeholder_keys[num_placeholders], key, sizeof(placeholder_keys[0]));
                        strncpy(placeholder_defaults[num_placeholders], default_value, sizeof(placeholder_defaults[0]));
                        num_placeholders++;
                    }

                    cursor = end_bracket + 1;
                }
            }

            // --- Ask user for placeholder values ---
            for (int i = 0; i < num_placeholders; i++)
            {
                printf("Customize %s (default: %s) for file %s? (leave empty for default): ",
                        placeholder_keys[i], placeholder_defaults[i], currentFile);
                fgets(user_values[i], sizeof(user_values[i]), stdin);
                user_values[i][strcspn(user_values[i], "\n")] = 0;

                if (strlen(user_values[i]) == 0)
                    strncpy(user_values[i], placeholder_defaults[i], sizeof(user_values[i]));
            }

            // Build const char* arrays
            const char *replacement_ptrs[MAX_PLACEHOLDERS];
            const char *placeholder_key_ptrs[MAX_PLACEHOLDERS];
            for (int i = 0; i < num_placeholders; i++)
            {
                replacement_ptrs[i] = user_values[i];
                placeholder_key_ptrs[i] = placeholder_keys[i];
            }

            // --- Rewind file section to write the file ---
            fseek(templateFile, file_pos, SEEK_SET);
            create_file_from_line(templateFile, currentFile, placeholder_key_ptrs, replacement_ptrs, num_placeholders);
        }
        else if (strncmp(line, "//", 3) == 0)
        {
            create_dir_from_line(line + 2, projectName);
        }
    }

    printf("\n✨ Project %s created using the %s template\n\n", projectName, templateName);
    fclose(templateFile);
}