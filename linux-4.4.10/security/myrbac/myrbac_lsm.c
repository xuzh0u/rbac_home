#include <linux/lsm_hooks.h>
#include <linux/sysctl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/types.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "~/rbac_home.h"

int is_enabled(void);
int alert_exec_result(int uid, int op);
int is_role_permitted(const char *role, const int op);
int is_user_permitted(int uid, int op);

static inline int my_isspace(const char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

int is_enabled(void)
{
    struct file *fout = filp_open(CONTROL_PATH, O_RDONLY, 0);
    char state_buf[sizeof(int)];
    int state = STAT_NA;
    mm_segment_t fs;

    if (!fout || IS_ERR(fout))
    {
        printk(_LSM_FMT "sorry, the module is damaged :(\n");
        return STAT_NA;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    vfs_read(fout, state_buf, sizeof(int), &fout->f_pos);
    memcpy(&state, state_buf, sizeof(int));

    set_fs(fs);
    filp_close(fout, NULL);

    return state;
}

int alert_exec_result(int uid, int op)
{
    if (is_user_permitted(uid, op) == STAT_YES)
    {
        printk(_USER_NAME_FMT ": success\n",
               uid);
        return STAT_YES;
    }
    else
    {
        printk(_USER_NAME_FMT ": fail\n",
               uid);
        return STAT_NO;
    }
}

int is_role_permitted(const char *role, const int op)
{
    char roles_full_path[MAX_PATH_LEN] = {ROLES_PATH};
    strcat(roles_full_path, role);
    struct file *fout = filp_open(roles_full_path, O_RDONLY, 0);
    char perm_buf[PERM_COUNT] = {0};
    mm_segment_t fs;
    int res = STAT_NA;

    strcat(roles_full_path, role);

    if (!fout || IS_ERR(fout))
    {
        printk(_ROLE_NAME_FMT
               " does not exist\n",
               role);
        return res;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    vfs_read(fout, perm_buf, sizeof(char) * PERM_COUNT, &fout->f_pos);

    printk(_ROLE_NAME_FMT, role);
    if (perm_buf[op] == '1')
    {
        printk(" is permitted\n");
        res = STAT_YES;
    }
    else if (perm_buf[op] == '0')
    {
        printk(" is NOT permitted\n");
        res = STAT_NO;
    }
    else
    {
        printk(": permission is corrupted\n");
    }

    set_fs(fs);
    filp_close(fout, NULL);

    return res;
}

int is_user_permitted(int uid, int op)
{
    if (uid <= 1000 || is_enabled() != STAT_YES)
        return STAT_YES; // permitted when root/not enable

    /* get role from uid */

    char uid_str[6] = {0};
    sprintf(uid_str, "%d", uid);
    char user_full_path[MAX_PATH_LEN] = {USERS_PATH};
    strcat(user_full_path, uid_str);

    struct file *fout = filp_open(user_full_path, O_RDONLY, 0);
    char role_buf[MAX_ROLENAME_LEN] = {0};
    mm_segment_t fs;

    if (!fout || IS_ERR(fout))
    {
        // no rule for uid, default 0000
        return STAT_NO;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    vfs_read(fout, role_buf, sizeof(char) * (MAX_ROLENAME_LEN), &fout->f_pos);

    int i;
    for (i = 0; i < MAX_ROLENAME_LEN; i++)
    {
        if (my_isspace(role_buf[i]))
        {
            role_buf[i] = 0;
        }
    }

    set_fs(fs);
    filp_close(fout, NULL);

    // check
    if (is_role_permitted(role_buf, op) != STAT_YES)
        return STAT_NO; // not permitted

    return STAT_YES; // permitted (normal case)
}

int myrbac_inode_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    printk(_LSM_FMT "<inode_create> exec by ");
    return alert_exec_result(current->real_cred->uid.val, OP_INODE_CREATE);
}

int myrbac_inode_rename(struct inode *old_inode, struct dentry *old_dentry,
                        struct inode *new_inode, struct dentry *new_dentry)
{
    printk(_LSM_FMT "<inode_rename> exec by ");
    return alert_exec_result(current->real_cred->uid.val, OP_INODE_RENAME);
}

int myrbac_inode_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    printk(_LSM_FMT "<inode_mkdir> exec by ");
    return alert_exec_result(current->real_cred->uid.val, OP_INODE_MKDIR);
}

int myrbac_inode_rmdir(struct inode *dir, struct dentry *dentry)
{
    printk(_LSM_FMT "<inode_rmdir> exec by ");
    return alert_exec_result(current->real_cred->uid.val, OP_INODE_RMDIR);
}

static struct security_hook_list myrbac_hooks[] = {
    LSM_HOOK_INIT(inode_rename, myrbac_inode_rename),
    LSM_HOOK_INIT(inode_create, myrbac_inode_create),
    LSM_HOOK_INIT(inode_mkdir, myrbac_inode_mkdir),
    LSM_HOOK_INIT(inode_rmdir, myrbac_inode_rmdir),
};

void __init myrbac_add_hooks(void)
{
    pr_info(_LSM_FMT "LSM LOADED.\n");
    security_add_hooks(myrbac_hooks, ARRAY_SIZE(myrbac_hooks));
}

static __init int myrbac_init(void)
{
    myrbac_add_hooks();
    return 0;
}

security_initcall(myrbac_init);