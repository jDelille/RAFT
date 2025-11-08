#ifndef TEMPLATES_H
#define TEMPLATES_H

#include <stdbool.h>

void install_template(const char *templatePath);

bool file_exists(const char *path);

void convert_windows_path_to_wsl(char *path);

void generate_project_from_template(
    const char *templateName,
    const char *projectName,
    const char **placeholder_keys,
    const char **replacement_values,
    int key_count);

int extract_placeholders_from_template(
    const char *templateName,
    char placeholder_keys[][128],
    char placeholder_defaults[][128],
    int max_placeholders);

#endif /* TEMPLATES_H */