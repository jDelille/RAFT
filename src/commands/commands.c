#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../scaffold/scaffold.h"
#include "../template/template.h"

void create_project_cmd()
{
    scaffold();
}

void install_template_cmd(const char *path)
{
    printf("Installing template: %s\n", path);
    install_template(path);
}

void copy_template_cmd() {
    copy_template();
}

void delete_template_cmd() {
    delete_template();
}

void clear_terminal_cmd() {
    system("clear");
    return;
}
