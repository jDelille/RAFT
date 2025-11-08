#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include "scaffold.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../template/template.h"
#include "../utils/defs.h"
#include "../utils/selection.h"
#include "../utils/utils.h"

/* List templates */
int list_templates(char templates[][256], int max_templates)
{
    DIR *d = opendir(".templates");
    if (!d)
        return 0;

    struct dirent *file;
    struct stat file_info;

    int count = 0;
    char path[512];

    while ((file = readdir(d)) != NULL && count < max_templates)
    {
        snprintf(path, sizeof(path), ".templates/%s", file->d_name);

        if (stat(path, &file_info) == 0 && S_ISREG(file_info.st_mode))
        {
            strncpy(templates[count], file->d_name, 256);
            templates[count][255] = '\0';
            count++;
        }
    }

    closedir(d);
    return count;
}

/* Step 1: Project Name */
void get_project_name(char *project_name, size_t size)
{
    printf("%s What is your project named? » ", ICON_QUESTION);
    fflush(stdout);

    fgets(project_name, size, stdin);
    project_name[strcspn(project_name, "\n")] = 0;

    CLEAR_PREV_LINE();

    printf("%s What is your project named? » %s\n", GREEN_CHECK, project_name);
}

/* Step 2: Project Template */
int choose_template(char *template_name, size_t size)
{
    char templates_list[64][256];
    int num_templates = list_templates(templates_list, 64);

    if (num_templates == 0)
    {
        printf("No templates found in .templates\n");
        printf("You need at least one template to create a project.\n");

        const char *yes_no[] = {"Yes", "No"};
        int install_choice = (selection("Would you like to install a template now?", yes_no, 2) == 0);

        if (install_choice)
        {
            char path[512];
            printf("Enter the path of your template file: ");

            if (fgets(path, sizeof(path), stdin))
            {
                sanitize_path(path);

                install_template(path);

                num_templates = list_templates(templates_list, 64);

                if (num_templates == 0)
                {
                    printf("No templates available after install. Aborting.\n");
                    template_name[0] = '\0';
                    return 0;
                }
            }
            else
            {
                printf("Input error. Aborting.\n");
                template_name[0] = '\0';
                return 0;
            }
        }
        else
        {
            printf("Aborting project creation.\n");
            template_name[0] = '\0';
            return 0;
        }
    }

    const char *template_names[64];
    for (int i = 0; i < num_templates; i++)
        template_names[i] = templates_list[i];

    int selected = selection("Which template do you want to use?", template_names, num_templates);

    strncpy(template_name, templates_list[selected], size);
    template_name[size - 1] = '\0';

    return 1;
}

int ask_to_customize()
{
    const char *yes_no[] = {"Yes", "No"};
    return selection("Do you want to customize placeholder values?", yes_no, 2) == 0;
}

int extract_placeholders(const char *template_name, char placeholder_keys[MAX_PLACEHOLDERS][128],
                         char placeholder_defaults[MAX_PLACEHOLDERS][128], char user_values[MAX_PLACEHOLDERS][128])
{
    return extract_placeholders_from_template(template_name, placeholder_keys, placeholder_defaults, MAX_PLACEHOLDERS);
}

// Function to prepare the replacement arrays
void prepare_replacement_arrays(int num_placeholders, const char placeholder_keys[MAX_PLACEHOLDERS][128],
                                const char user_values[MAX_PLACEHOLDERS][128], const char **key_ptrs,
                                const char **replacement_ptrs)
{
    for (int i = 0; i < num_placeholders; i++)
    {
        replacement_ptrs[i] = user_values[i];
        key_ptrs[i] = placeholder_keys[i];
    }
}

void scaffold()
{
    char project_name[256];
    char template_name[256];

    get_project_name(project_name, sizeof(project_name));

    if (!choose_template(template_name, sizeof(template_name)))
    {
        return; // No template selected, exit
    }

    int customize = ask_to_customize();

    char placeholder_keys[MAX_PLACEHOLDERS][128];
    char placeholder_defaults[MAX_PLACEHOLDERS][128];
    char user_values[MAX_PLACEHOLDERS][128];

    int num_placeholders = extract_placeholders(template_name, placeholder_keys, placeholder_defaults, user_values);
    if (num_placeholders == 0)
    {
        printf("No placeholders detected in template %s\n", template_name);
    }

    const char *replacement_ptrs[MAX_PLACEHOLDERS];
    const char *key_ptrs[MAX_PLACEHOLDERS];
    prepare_replacement_arrays(num_placeholders, placeholder_keys, user_values, key_ptrs, replacement_ptrs);

    generate_project_from_template(
        template_name,
        project_name,
        customize);
}