#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>

#include "../utils/defs.h"
#include "../utils/utils.h"
#include "../utils/selection.h"
#include "../scaffold/scaffold.h"

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
        // Stop at next file section
        if (strncmp(input_line, "// ", 3) == 0)
        {
            fseek(template_fp, -strlen(input_line), SEEK_CUR);
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

    // Ensure the file has normal permissions
    chmod(output_path, 0644); // owner rw, group r, others r
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

/* Copy a template */
void copy_template()
{
    char templates_list[64][256];

    int num_templates = list_templates(templates_list, 64);

    const char *template_names[64];
    for (int i = 0; i < num_templates; i++)
    {
        template_names[i] = templates_list[i];
    }
    int template_selection = selection("Which template do you want to copy?", template_names, num_templates);

    if (template_selection < 0)
    {
        return;
    }

    const char *filename = template_names[template_selection];
    char src_path[512];
    snprintf(src_path, sizeof(src_path), ".templates/%s", filename);

    char new_name[512];
    printf("Enter a new filename for the copied template: ");
    if (scanf("%255s", new_name) != 1)
    {
        printf("Invalid input.\n");
        return;
    }

    new_name[sizeof(new_name) - 1] = '\0';
    if (strlen(new_name) > 200)
    {
        printf("Error: template name too long.\n");
        return;
    }

    char dest_path[1024];

    snprintf(dest_path, sizeof(dest_path), ".templates/%s.tmpl", new_name);

    if (file_exists(dest_path))
    {
        printf("Error: A template named '%s' already exists.\n", new_name);
        return;
    }

    if (copy_file(src_path, dest_path))
        printf("Template copied successfully to %s\n", dest_path);
    else
        printf("Failed to copy template.\n");
}

/* Delete a template */
void delete_template()
{
    while (1)
    {
        char templates_list[64][256];
        int num_templates = list_templates(templates_list, 64);

        if (num_templates == 0)
        {
            printf("No templates found to delete.\n");
            return;
        }

        // Build template name array (+1 for "Quit")
        const char *template_names[65];
        for (int i = 0; i < num_templates; i++)
        {
            template_names[i] = templates_list[i];
        }
        template_names[num_templates] = "Quit";

        int selected = selection("Which template do you want to delete?", template_names, num_templates + 1);

        // Handle Quit
        if (selected == num_templates || selected < 0)
        {
            printf("Exiting template deletion.\n");
            break;
        }

        const char *selected_template = template_names[selected];

        // Confirmation prompt
        const char *yes_no[] = {"Yes", "No"};
        int confirm = selection("Are you sure you want to delete this template?", yes_no, 2);

        if (confirm != 0) // 0 = "Yes", 1 = "No"
        {
            printf("Deletion canceled.\n");
            continue; // back to main loop
        }

        char path[512];
        snprintf(path, sizeof(path), ".templates/%s", selected_template);

        if (remove(path) == 0)
        {
            printf("Template '%s' deleted successfully.\n", selected_template);
        }
        else
        {
            perror("Error deleting template");
        }

        printf("\n"); // visual spacing before next loop
    }
}

/* Generate a new file from a template */
void generate_project_from_template(const char *templateName, const char *projectName, bool customize)
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
        // Skip comment headers // # ...
        if (strncmp(line, "// #", 4) == 0)
            continue;

        size_t len = strlen(line);

        // Directory line (ends with '/')
        if (strncmp(line, "//", 2) == 0 && line[len - 2] == '/')
        {
            create_dir_from_line(line + 2, projectName);
            continue;
        }

        // File line (does not end with '/')
        if (strncmp(line, "//", 2) == 0 && line[len - 2] != '/')
        {
            char filePath[512];
            snprintf(filePath, sizeof(filePath), "%s/%s", projectName, line + 3);
            filePath[strcspn(filePath, "\n")] = 0;
            printf("\n");
            printf("%s\n", line); // Print file header

            //  Collect placeholders for this file
            char placeholder_keys[MAX_PLACEHOLDERS][128];
            char placeholder_defaults[MAX_PLACEHOLDERS][128];
            char user_values[MAX_PLACEHOLDERS][128];
            int num_placeholders = 0;

            long section_start = ftell(templateFile); // start of file section
            char section_line[1024];

            while (fgets(section_line, sizeof(section_line), templateFile))
            {
                if (strncmp(section_line, "//", 2) == 0) // Next file or directory
                {
                    fseek(templateFile, -strlen(section_line), SEEK_CUR);
                    break;
                }

                const char *cursor = section_line;
                while ((cursor = strchr(cursor, '[')) != NULL)
                {
                    const char *end_bracket = strchr(cursor, ']');
                    if (!end_bracket)
                        break;

                    char content[256];
                    size_t len = end_bracket - cursor - 1;
                    if (len >= sizeof(content))
                        len = sizeof(content) - 1;
                    strncpy(content, cursor + 1, len);
                    content[len] = '\0';

                    char *equal_sign = strchr(content, '=');
                    char key[128] = "";
                    char default_val[128] = "";

                    if (equal_sign)
                    {
                        *equal_sign = '\0';
                        strncpy(key, content, sizeof(key));
                        strncpy(default_val, equal_sign + 1, sizeof(default_val));
                    }
                    else
                    {
                        strncpy(key, content, sizeof(key));
                        default_val[0] = '\0';
                    }

                    key[strcspn(key, " \r\n\t")] = 0;
                    default_val[strcspn(default_val, " \r\n\t")] = 0;

                    if (strlen(key) == 0)
                    {
                        cursor = end_bracket + 1;
                        continue;
                    }

                    bool exists = false;
                    for (int i = 0; i < num_placeholders; i++)
                        if (strcmp(placeholder_keys[i], key) == 0)
                            exists = true;

                    if (!exists && num_placeholders < MAX_PLACEHOLDERS)
                    {
                        strncpy(placeholder_keys[num_placeholders], key, sizeof(placeholder_keys[0]));
                        strncpy(placeholder_defaults[num_placeholders], default_val, sizeof(placeholder_defaults[0]));
                        num_placeholders++;
                    }

                    cursor = end_bracket + 1;
                }
            }

            if (customize) // only prompt if user wants to customize
            {
                for (int i = 0; i < num_placeholders; i++)
                {
                    printf("%s (default: %s): ", placeholder_keys[i], placeholder_defaults[i]);
                    fgets(user_values[i], sizeof(user_values[i]), stdin);
                    user_values[i][strcspn(user_values[i], "\n")] = 0;

                    if (strlen(user_values[i]) == 0)
                        strncpy(user_values[i], placeholder_defaults[i], sizeof(user_values[i]));
                }
            }
            else
            {
                // just copy defaults
                for (int i = 0; i < num_placeholders; i++)
                    strncpy(user_values[i], placeholder_defaults[i], sizeof(user_values[i]));
            }

            // Build pointer arrays for replacement
            const char *replacement_ptrs[MAX_PLACEHOLDERS];
            const char *key_ptrs[MAX_PLACEHOLDERS];
            for (int i = 0; i < num_placeholders; i++)
            {
                replacement_ptrs[i] = user_values[i];
                key_ptrs[i] = placeholder_keys[i];
            }

            // Generate file
            fseek(templateFile, section_start, SEEK_SET);
            create_file_from_line(templateFile, filePath, key_ptrs, replacement_ptrs, num_placeholders);
        }
    }

    printf("\n✨ Project %s created using the %s template\n\n", projectName, templateName);
    fclose(templateFile);
}