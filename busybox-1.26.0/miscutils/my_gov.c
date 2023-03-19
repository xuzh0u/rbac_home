#include "libbb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "~/rbac_home.h"

int state_get();
int state_set(int state);
int role_set_perm(const char *role, const char *perm);
int role_delete(const char *role);
int role_show();
int user_set_role(const char *uid, const char *role);
int user_show();
void alert_usage();

int state_get()
{
    FILE *fp = fopen(CONTROL_PATH, "rb");
    int state;

    if (!fp)
    {
        printf("file open failed\n");
        return STAT_NA;
    }

    fread(&state, sizeof(int), 1, fp);

    return state;
}

int state_set(int state)
{
    FILE *fp = fopen(CONTROL_PATH, "wb");

    if (!fp)
    {
        printf("file open failed\n");
        return STAT_NA;
    }

    fwrite(&state, sizeof(int), 1, fp);

    return STAT_YES;
}

int role_set_perm(const char *role, const char *perm)
{
    char role_full_path[MAX_PATH_LEN] = {ROLES_PATH};
    strcat(role_full_path, role);
    char cmd[100] = {"echo "};
    strcat(cmd, perm);
    strcat(cmd, " > ");
    strcat(cmd, role_full_path);
    system(cmd);
    return STAT_YES;
}

int role_delete(const char *role)
{
    // delete role
    char cmd_d[MAX_CMD_LEN] = {"rm -f " ROLES_PATH};
    strcat(cmd_d, role);
    system(cmd_d);
    // delete users record
    char cmd[MAX_CMD_LEN] = {"for file in ./* ; do if [ `cat $file` == \""};
    strcat(cmd, role);
    char *cmd1 = "\" ]; then  rm $file; fi; done";
    strcat(cmd, cmd1);
    system(cmd);
    return STAT_YES;
}

int role_show()
{
    // show role-perm pairs (1-n)
    printf("role\tpermission\n");
    char *cmd = "for file in " ROLES_PATH "* ; do echo ${file##*/}\"\t\"`cat $file` ; done";
    system(cmd);
    return STAT_YES;
}
int user_set_role(const char *uid, const char *role)
{
    char user_full_path[MAX_PATH_LEN] = {USERS_PATH};
    strcat(user_full_path, uid);

    char cmd[100] = {"echo "};
    strcat(cmd, role);
    strcat(cmd, " > ");
    strcat(cmd, user_full_path);
    system(cmd);
    return STAT_YES;
}

int user_show()
{
    // show user-role pairs (1-1)
    printf("user role\n");
    char *cmd = "for file in " USERS_PATH "* ; do echo ${file##*/}\t`cat $file` ; done";
    system(cmd);
    return STAT_YES;
}
void alert_usage()
{
    printf("Usage: my_gov [options] [arguments]\n\n");
    printf("\t\t-h \t\t\t\tsee help\n");

    printf("\n");

    printf("\t\t-s \t\t\t\tcheck module status\n");
    printf("\t\t-s <enable/disable>\t\tenable/disable this module\n");

    printf("\n");

    printf("\t\t-u \t\t\t\tlist user-role pairs\n");
    printf("\t\t-u <uid> <role name>\t\tset user <uid> as <role name>\n");

    printf("\n");

    printf("\t\t-r \t\t\t\tlist role-permission pairs\n");
    printf("\t\t-r <role name> <4 bits>\t\tlet role <role name> has permission <4 bits>\n");
    printf("\t\t-r <role name> d\t\tdelete role <role name>\n");
}

int my_gov_main(int argc UNUSED_PARAM, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0)
    {
        alert_usage();
    }
    else if (strcmp(argv[1], "-s") == 0) // status
    {
        if (argc == 2)
        {
            int state = state_get();
            if (state == STAT_YES)
                printf("Module is enabled\n");
            else if (state == STAT_NO)
                printf("Module is disabled\n");
            else
                printf("Module is not installed\n");
        }
        else if (strcmp(argv[2], "enable") == 0)
        {
            if (state_set(STAT_YES) == STAT_YES)
                printf("Module enabled successfully\n");
            else
                printf("Module enable failed\n");
        }
        else if (strcmp(argv[2], "disable") == 0)
        {
            if (state_set(STAT_NO) == STAT_YES)
                printf("Module disabled successfully\n");
            else
                printf("Module disable failed\n");
        }
        else
        {
            alert_usage();
        }
    }
    else if (strcmp(argv[1], "-u") == 0) // user
    {
        if (argc == 2)
        {
            user_show();
        }
        else if (argc == 4) // argv[2] is uid, argv[3] is role
        {
            for (int i = 0; i < strlen(argv[2]); i++) // check uid fmt
            {
                if (!isdigit(argv[2][i]))
                {
                    printf("[ERROR] uid in BAD format (expect digits)\n");
                    return 0;
                }
            }
            if (argv[2] == "1000") // check if root
            {
                printf("[ERROR] sorry, root[uid: 1000] cannot be modified for safety\n");
                return 0;
            }
            if (strlen(argv[3]) > MAX_ROLENAME_LEN) // check rolename length
            {
                printf("[ERROR] role's name TOO LONG (expect less than %d)\n", MAX_ROLENAME_LEN);
                return 0;
            }

            if (user_set_role(argv[2], argv[3]) == STAT_YES)
                printf("The role of user added successfully\n");
            else
                printf("The role of user add failed\n");
        }
        else
        {
            alert_usage();
        }
    }
    else if (strcmp(argv[1], "-r") == 0) // role
    {
        if (argc == 2)
        {
            role_show();
        }
        else if (argc == 4) // argv[2] is role, argv[3] is permission or 'd'
        {
            if (argv[2] == "0") // check if reserved role for root
            {
                printf("[ERROR] sorry, role[0] is reserved for root\n");
                return 0;
            }
            if (strlen(argv[2]) > MAX_ROLENAME_LEN) // check rolename length
            {
                printf("[ERROR] role's name TOO LONG (expect less than %d)\n", MAX_ROLENAME_LEN);
            }
            if (strlen(argv[3]) != PERM_COUNT) // check perm length
            {
                printf("[ERROR] permission in BAD format (expect 4 digits)\n");
                return 0;
            }
            for (int i = 0; i < strlen(argv[3]); i++) // check perm fmt
            {
                if (argv[3][i] > '1' || argv[3][i] < '0')
                {
                    printf("[ERROR] permission in BAD format (expect 0/1 digits)\n");
                    return 0;
                }
            }

            if (strcmp(argv[3], "d") == 0) // delete role
            {
                if (role_delete(argv[2]) == STAT_YES)
                    printf("Role deleted successfully\n");
                else
                    printf("Role delete failed\n");
            }
            else // set role
            {
                if (role_set_perm(argv[2], argv[3]) == STAT_YES)
                    printf("Role added successfully\n");
                else
                    printf("Role add failed\n");
            }
        }
        else
        {
            alert_usage();
        }
    }
    else
    {
        alert_usage();
    }
    return 0;
}
