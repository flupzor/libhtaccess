#include "htaccess/htaccess.h"
#include "htaccess_internal.h"


int
htaccess_parse_directives(const char *buf, htaccess_file_t *hta_file) {
    unsigned int i;
    char *str;
    htaccess_directive_map_t *hta_dir_map;
    htaccess_directive_kv_t *hta_dir_kv;
    htaccess_directive_value_t *hta_dir_val;

    for (i = 0; i < strlen(buf); i++) {
        if (buf[i] == '\n') {
            continue;
        } else if (buf[i] == ' ' || buf[i] == '\t') {
            continue;
        } else if (strncasecmp("</files>", &buf[i], strlen("</files>")) == 0) {
            /* Found parent, backstep by one and return */
            /* printf("Found </files>, returning - %d\n", *lineno); */
            return i - 1;
        } else if ((hta_dir_map = search_directive_map_on_str(&buf[i]))) {
            /* Based on the type of dir_map, track how many parameters it expects
             * and push the parsed data into the right type of struct and tree/list
             */
            /* printf("------------ I found a \"%s\", par_cnt: %d----------------------\n", hta_dir_map->str, hta_dir_map->par_cnt); */
            hta_dir_kv = new_htaccess_directive_kv(hta_dir_map);
            if (!hta_dir_kv)
                return -1;

            /* Move beyond the matched tag */
            /* i += hta_dir_map->len; */

            /* Parse and extract the parameter count based on the directive */
            /* for (j = 0; j < hta_dir_map->par_cnt; j++) { */

            for (i += hta_dir_map->len; buf[i] != '\n'; i++) {
                /* Move beyond the whitespace */
                if ((buf[i] == '\t') || (buf[i] == ' ')) {
                    continue;
                }

                if (buf[i] == '\"') {
                    str = htaccess_parse_quoted_string(&buf[i]);
                    i += strlen(str) + 2 - 1;
                } else {
                    str = htaccess_copy_string(&buf[i]);
                    i += strlen(str) - 1;
                }

                hta_dir_val = new_htaccess_directive_value(str, 0);
                if (!hta_dir_val) {
                    free_htaccess_directive_kv(hta_dir_kv);
                    return -1;
                }
                TAILQ_INSERT_TAIL(&(hta_dir_kv->values), hta_dir_val, next);
            }
            RB_INSERT(rb_directive_kv_list_head_t, &(hta_file->directives), hta_dir_kv);
            continue;
        } else {
            printf("Unknown directive\n");
            printf("---->%.70s<----\n", &buf[i]);

            return -1;
        }
    }
    return i;
}


int
htaccess_parse_files(const char *buf, htaccess_directory_t *hta_dir) {
    unsigned int i;
    int rc;
    enum parser_state_e state = NONE;
    char *str;
    htaccess_file_t *hta_file;

    for (i = 0; i < strlen(buf); i++) {
        if (buf[i] == '\n') {
            continue;
        } else if (buf[i] == ' ' || buf[i] == '\t') {
            continue;
        } else if (strncasecmp("</directory>", &buf[i], strlen("</directory>")) == 0) {
            /* Found parent, backstep by one and return */
            /* printf("Found </Directory> (in files), returning - %d\n", *lineno); */
            return i - 1;
        } else if (strncasecmp("</files>", &buf[i], strlen("</files>")) == 0) {
            i += strlen("</files>");
            state = NONE;

            /* printf("RB_INSERT(rb_file_list_head_t, &(hta_dir->files), hta_file);\n"); */
            if (!hta_file) {
                printf("FAIL! no hta_dir\n");
                return -1;
            }
            RB_INSERT(rb_file_list_head_t, &(hta_dir->files), hta_file);
            hta_file = NULL;

            /* printf("State change to NONE (files) - %d\n", *lineno); */

            continue;

        } else if (strncasecmp("<files", &buf[i], strlen("<files")) == 0) {
            i += strlen("<files");
            state = IN_TAG_FILES;
            /* printf("State change to IN_TAG_FILES - %d\n", *lineno); */

            hta_file = new_htaccess_file();
            if (!hta_file) {
                printf("Unable to construct a new file entry\n");
                return -1;
            }

            continue;
        } else if (state == IN_TAG_FILES && buf[i] == '>') {
            state = IN_CTX_FILES;
            /* printf("State change to IN_CTX_FILES - %d\n", *lineno); */
            continue;
        } else if (state == IN_TAG_FILES && buf[i] == '\"') {
            str = htaccess_parse_quoted_string(&buf[i]);
            /* printf("- %s - %d\n", str, *lineno); */
            i += strlen(str) + 2;

            if (!hta_file) {
                printf("Unable to find a file entry\n");
                return -1;
            }
            hta_file->filename = str;

            continue;
        } else if (state == IN_CTX_FILES) {
            /* Parse directives */
            rc = htaccess_parse_directives(&buf[i], hta_file);
            if (rc < 0)
                return -1;

            i += rc;
            continue;
        } else {
            printf("Parse error: \"%.20s\n", &buf[i]);
            return -1;
        }
    }
    return i;
}

int
htaccess_parse_directory(const char *buf,
                        htaccess_ctx_t *ht_ctx) {
    unsigned int i;
    int rc;
    enum parser_state_e state = NONE;
    char *str;
    htaccess_directory_t *hta_dir;


    for (i = 0; i < strlen(buf); i++) {
        if (buf[i] == '\n') {
            ht_ctx->lineno++;
            continue;
        } else if (buf[i] == ' ' || buf[i] == '\t') {
            continue;
        } else if (strncasecmp("</directory>", &buf[i], strlen("</directory>")) == 0) {
            i += strlen("</directory>");
            ht_ctx->indent--;
            state = NONE;

            /* printf("RB_INSERT(rb_directory_list_head_t, &(ht_ctx->directories), dir)\n"); */
            if (!hta_dir) {
                printf("FAIL! no hta_dir\n");
                return -1;
            }

            RB_INSERT(rb_directory_list_head_t, &(ht_ctx->directories), hta_dir);
            hta_dir = NULL;

            /* printf("State change to NONE (directory) - %d\n", *lineno); */

            continue;
        } else if (strncasecmp("<directory", &buf[i], strlen("<directory")) == 0) {
            i += strlen("<directory");
            ht_ctx->indent++;
            state = IN_TAG_DIRECTORY;
            /* printf("State change to IN_TAG_DIRECTORY - %d\n", *lineno); */

            hta_dir = new_htaccess_directory();
            if (!hta_dir) {
                printf("Unable to construct a new directory entry\n");
                return -1;
            }

            continue;
        } else if (state == IN_TAG_DIRECTORY && buf[i] == '\"') {
            str = htaccess_parse_quoted_string(&buf[i]);
            if (!hta_dir) {
                printf("Parse error: htaccess_directory_t was not created\n");
                return -1;
            }

            hta_dir->dirname = str;
            /* printf("- %s - %d\n", str, *lineno); */
            i += strlen(str) + 2;

            continue;
        } else if (state == IN_TAG_DIRECTORY && buf[i] == '>') {
            state = IN_CTX_DIRECTORY;
            /* printf("State change to IN_CTX_DIRECTORY - %d\n", *lineno); */
            continue;
        } else if (state == IN_CTX_DIRECTORY) {
            /* Parse the Files */
            rc = htaccess_parse_files(&buf[i], hta_dir);
            if (rc < 0)
                return -1;

            i += rc;
            continue;
        } else {
            printf("Parse error: \"%.20s\"\n", &buf[i]);
            return -1;
        }
    }
    printf("I'm done\n");
    return 0;
}

int
htaccess_parse_htgroup(htaccess_filepath_t *hta_path) {
    size_t i, il;
    char *buf, *line, *username, *groupname;
    htaccess_htgroup_t *gr;


    if (!hta_path || !hta_path->path || hta_path->done)
        return 1;

    /* buf = htaccess_readfile(hta_path->path); */
    buf = htaccess_readfile("test/group0");
    if (!buf)
        return 1;

    for (i = 0; buf != '\0'; i++) {
        if (buf[i] == '\n')
            continue;

        line = htaccess_str_returned_upto_EOL(&buf[i]);
        if (!line || strlen(line) == 0)
            break;

        groupname = htaccess_str_returned_upto_colon(line);
        il = strlen(groupname) + 1;

        while (line[il] != '\0') {
            if (line[il] == '\t' || line[il] == ' ') {
                il++;
                continue;
            }

            username = htaccess_copy_string(&line[il]);
            if (!username)
                break;

            gr = new_htaccess_htgroup();
            gr->groupname = groupname;
            gr->username = username;

            RB_INSERT(rb_htgroup_tree_t, &(hta_path->htgroup), gr);
            il += strlen(username);
        }

        i += strlen(line) - 1;
        free(line);
    }

    hta_path->done = 1;
    return 0;
}

int
htaccess_parse_htpasswd(htaccess_filepath_t *hta_path) {
    size_t i, il;
    char *buf, *line, *username, *pwhash;
    htaccess_htpasswd_t *pw;

    if (!hta_path || !hta_path->path || hta_path->done)
        return 1;

    printf("Should read %s, going to read /tmp/htpasswd\n", hta_path->path);
    buf = htaccess_readfile(hta_path->path);
    /* buf = htaccess_readfile("/tmp/htpasswd"); */
    if (!buf)
        return 1;

    for (i = 0; buf != '\0'; i++) {
        if (buf[i] == '\n')
            continue;

        line = htaccess_str_returned_upto_EOL(&buf[i]);
        if (!line || strlen(line) == 0)
            break;

        username = htaccess_str_returned_upto_colon(line);
        il = strlen(username) + 1;
        pwhash = strdup(&line[il]);

        pw = new_htaccess_htpasswd();
        pw->username = username;
        pw->pwhash   = pwhash;

        RB_INSERT(rb_htpasswd_tree_t, &(hta_path->htpasswd), pw);

        i += strlen(line) - 1;
        free(line);
    }

    hta_path->done = 1;
    return 0;
}

void
htaccess_print_ctx(htaccess_ctx_t *ht_ctx) {
    htaccess_directory_t *hta_dir;
    htaccess_file_t *hta_file;
    htaccess_directive_kv_t *hta_dir_kv;
    htaccess_directive_value_t *hta_dir_value;
    htaccess_htpasswd_t *pw;
    htaccess_htgroup_t *gr;
    htaccess_filepath_t *hta_filepath;


    RB_FOREACH(hta_dir, rb_directory_list_head_t, &(ht_ctx->directories)) {
        printf("hta_dir->dirname: %s\n", hta_dir->dirname);
        RB_FOREACH(hta_file, rb_file_list_head_t, &(hta_dir->files)) {
            printf("\thta_file->filename: %s\n", hta_file->filename);
            RB_FOREACH(hta_dir_kv, rb_directive_kv_list_head_t, &(hta_file->directives)) {
                printf("\t\thta_dir_kv->key->str: %s\n", hta_dir_kv->key->str);
                TAILQ_FOREACH(hta_dir_value, &(hta_dir_kv->values), next) {
                    printf("\t\t\thta_dir_value->value: %s\n", hta_dir_value->value);

                    if (hta_dir_kv->key->type == AUTHUSERFILE) {
                        RB_FOREACH(pw, rb_htpasswd_tree_t, &(hta_dir_value->filepath->htpasswd)) {
                            printf("\t\t\t\tusername: \"%s\" password hash: \"%s\"\n",
                                    pw->username,
                                    pw->pwhash);
                        }
                    } else if (hta_dir_kv->key->type == AUTHGROUPFILE) {
                        RB_FOREACH(gr, rb_htgroup_tree_t, &(hta_dir_value->filepath->htgroup)) {
                            printf("\t\t\t\tgroupname: \"%s\" username: \"%s\"\n",
                                    gr->groupname,
                                    gr->username);
                        }
                    }
                }
            }
        }
    }
    RB_FOREACH(hta_filepath, rb_filepath_tree_t, &(ht_ctx->paths)) {
        printf("path: %s\n", hta_filepath->path);
    }

    return;
}
