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
void get_name(char *project_name, size_t size)
{
    printf("%s What is your project named? » ", ICON_QUESTION);
    fflush(stdout);

    fgets(project_name, size, stdin);
    project_name[strcspn(project_name, "\n")] = 0;

    CLEAR_PREV_LINE();

    printf("%s What is your project named? » %s\n", GREEN_CHECK, project_name);
}

/* Step 2: Project Template */
void choose_template(char *template_name, size_t size)
{
    char templates_list[64][256];
    int num_templates = list_templates(templates_list, 64);

    if (num_templates == 0)
    {
        printf("No templates found in .templates\n");
        template_name[0] = '\0';
        return;
    }

    const char *template_names[64];
    for (int i = 0; i < num_templates; i++)
        template_names[i] = templates_list[i];

    int selected = selection("Which template do you want to use?", template_names, num_templates);

    strncpy(template_name, templates_list[selected], size);
    template_name[size - 1] = '\0';
}

/* Step 3: Additional files */
void add_additional_files()
{
    const char *yes_no[] = {"Yes", "No"};
    int create_more = selection("Do you want to generate additional files?", yes_no, 2);

    if (create_more)
    {
        printf("Hey");
    }
}

void scaffold()
{
    char project_name[256];
    char template_name[256];

    // Step 1: Ask for project name
    get_name(project_name, sizeof(project_name));

    // Step 2: Choose a template
    choose_template(template_name, sizeof(template_name));

    // Step 3: Ask if user wants to customize placeholders
    const char *yes_no[] = {"Yes", "No"};
    int customize = selection("Do you want to customize placeholder values?", yes_no, 2);

    // Step 4: Extract placeholders from template
    #define MAX_PLACEHOLDERS 20
    char placeholder_keys[MAX_PLACEHOLDERS][128];
    char placeholder_defaults[MAX_PLACEHOLDERS][128];
    char user_values[MAX_PLACEHOLDERS][128];

    int num_placeholders = extract_placeholders_from_template(
        template_name,
        placeholder_keys,
        placeholder_defaults,
        MAX_PLACEHOLDERS
    );

    if (num_placeholders == 0)
    {
        printf("No placeholders detected in template %s\n", template_name);
    }

    // Step 5: Prompt user for each placeholder if customization is enabled
    for (int i = 0; i < num_placeholders; i++)
    {
        if (!customize)
        {
            printf("Customize %s (default: %s)? (leave empty for default): ",
                   placeholder_keys[i], placeholder_defaults[i]);
            fgets(user_values[i], sizeof(user_values[i]), stdin);
            user_values[i][strcspn(user_values[i], "\n")] = 0;

            if (strlen(user_values[i]) == 0)
            {
                strncpy(user_values[i], placeholder_defaults[i], sizeof(user_values[i]));
            }
        }
        else
        {
            strncpy(user_values[i], placeholder_defaults[i], sizeof(user_values[i]));
        }
    }

    // Step 6: Convert to const char * arrays for generator
    const char *replacement_ptrs[MAX_PLACEHOLDERS];
    const char *key_ptrs[MAX_PLACEHOLDERS];
    for (int i = 0; i < num_placeholders; i++)
    {
        replacement_ptrs[i] = user_values[i];
        key_ptrs[i] = placeholder_keys[i];
    }

    // Step 7: Generate project from template
    generate_project_from_template(template_name, project_name, key_ptrs, replacement_ptrs, num_placeholders);

    // Optional: add additional files step can go here
}