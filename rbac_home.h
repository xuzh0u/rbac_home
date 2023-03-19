#define USERS_PATH "/etc/myrbac_LSM/users/"
#define ROLES_PATH "/etc/myrbac_LSM/roles/"
#define CONTROL_PATH "/etc/myrbac_LSM/stat"
#define USER_ROOT_DEFAULT_ROLE "0"
#define ROLE_ZERO_DEFAULT_PERM "1111"
#define PERM_COUNT 4
#define MAX_CMD_LEN 200
#define MAX_PATH_LEN 50
#define MAX_ROLENAME_LEN 20
#define MAX_USERNAME_LEN 20

#define _USER_NAME_FMT "user[uid: %d]"
#define _ROLE_NAME_FMT "role[%s]"
#define _LSM_FMT "[[myrbac_LSM]] "

enum ops
{
    OP_INODE_CREATE = 0,
    OP_INODE_RENAME = 1,
    OP_INODE_MKDIR = 2,
    OP_INODE_RMDIR = 3,
};

enum stats
{
    STAT_NO = -1,
    STAT_YES = 0,
    STAT_NA = 1
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
