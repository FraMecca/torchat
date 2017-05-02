main() <int main (int argc, char **argv) at settings.c:29>:
    fprintf()
    exit()
    strcmp()
    start_daemon() <void start_daemon () at main.c:39>:
        fork()
        exit()
        setsid()
        signal()
        umask()
        CALLOC()
        getcwd()
        chdir()
        free()
        sysconf()
        close()
    atoi()
    signal()
    dumpstack() <void dumpstack (int sig) at util.c:42>:
        sprintf()
        getpid()
        system()
        exit()
    log_init()
    exit_from_signal() <void exit_from_signal (int sig) at main.c:267>:
        fprintf()
    read_tor_hostname() <char *read_tor_hostname (void) at main.c:92>:
        fopen()
        exit_error() <void exit_error (char *s) at util.c:12>:
            perror()
            exit()
        fscanf()
        fclose()
        STRDUP()
    initialize_proxy_connection() <void initialize_proxy_connection (const char *host, const int port) at socks_helper.c:106>:
        proxysocket_initialize()
        proxysocketconfig_create_direct()
        proxysocketconfig_use_proxy_dns()
        proxysocketconfig_add_proxy()
    bind_and_listen() <int bind_and_listen (const int portno, int n) at socks_helper.c:62>:
        socket()
        exit_error() <void exit_error (char *s) at util.c:12>:
            perror()
            exit()
        bzero()
        htons()
        bind()
        fcntl()
        listen()
    event_loop() <void event_loop (const int listenSock) at main.c:172>:
        epoll_create1()
        assert()
        epoll_ctl()
        CALLOC()
        epoll_wait()
        yield()
        close()
        accept()
        perror()
        torchatproto_fd_unblock() <int torchatproto_fd_unblock (int s) at torchatproto.c:98>:
            fcntl()
            assert()
            setsockopt()
        go()
        event_routine() <coroutine void event_routine (const int rawsock) at main.c:107>:
            torchatproto_attach() <int torchatproto_attach (int s) at torchatproto.c:117>:
                MALLOC()
                torchatproto_hquery() <void *torchatproto_hquery (struct hvfs hvfs, const void *type) at torchatproto.c:151>:
                close()
                torchatproto_hclose() <void torchatproto_hclose (struct hvfs hvfs) at torchatproto.c:143>:
                    FREE()
                torchatproto_hdone() <int torchatproto_hdone (struct hvfs hvfs, int64_t deadline) at torchatproto.c:92>
                torchatproto_msendl() <int torchatproto_msendl (struct msock_vfs mvfs, struct iolist first, struct iolist last, int64_t deadline) at torchatproto.c:44>:
                    CONT()
                    fd_send()
                torchatproto_mrecvl() <ssize_t torchatproto_mrecvl (struct msock_vfs mvfs, struct iolist first, struct iolist last, int64_t deadline) at torchatproto.c:59>:
                    CONT()
                    recv()
                    fcntl()
                    assert()
                    fdin()
                hmake()
                FREE()
            now()
            parse_connection() <int parse_connection (const int sock, struct data_wrapper retData, char **retJson, int64_t deadline) at actions.c:24>:
                memset()
                torchatproto_mrecv() <ssize_t torchatproto_mrecv (int h, void *buf, size_t maxLen, int64_t deadline) at torchatproto.c:265>:
                    hquery()
                    CONT()
                    fd_recv_dimensionhead() <int fd_recv_dimensionhead (struct msock_vfs mvfs, int64_t deadline) at torchatproto.c:241>:
                        CONT()
                        recv()
                        isdigit()
                        strtol()
                    recv()
                    yield()
                    now()
                    assert()
                strlen()
                CALLOC()
                strncpy()
                convert_string_to_datastruct()
                log_err()
            log_info()
            free_data_wrapper() <void free_data_wrapper (struct data_wrapper data) at actions.c:61>:
                FREE()
            store_msg() <void store_msg (struct data_wrapper data) at actions.c:137>:
                peer_exist() <bool peer_exist (const char *id) at datastruct.c:47>:
                    get_peer() <struct peer get_peer (const char *id) at datastruct.c:39>
                insert_peer() <bool insert_peer (const char *onion) at datastruct.c:27>:
                    new_peer() <struct peer new_peer (const char *id) at datastruct.c:14>
                    HASH_ADD_STR()
                insert_new_message() <bool insert_new_message (const char *peerId, const char *content, enum command cmd) at datastruct.c:85>:
                    get_peer() <struct peer get_peer (const char *id) at datastruct.c:39>
                    get_short_date() <char *get_short_date () at util.c:32>:
                        time()
                        localtime()
                        strftime()
                        STRDUP()
                    new_message() <struct message new_message (const char *content, char *date, enum command cmd) at datastruct.c:57>
                    FREE()
                    get_tail() <struct message get_tail (struct message h) at datastruct.c:75>
            relay_msg() <void relay_msg (const int clientSock, struct data_wrapper data, int64_t deadline) at actions.c:129>:
                assert()
                send_routine() <void send_routine (const int clientSock, struct data_wrapper data, int64_t deadline) at actions.c:90>:
                    STRDUP()
                    strcpy()
                    convert_datastruct_to_char()
                    send_over_tor() <int send_over_tor (const char *domain, const int port, char *buf, int64_t deadline) at socks_helper.c:166>:
                        MALLOC()
                        pthread_create()
                        open_socket_to_domain_thread() <void *open_socket_to_domain_thread (void *domainDataVoid) at socks_helper.c:150>:
                            proxysocket_connect()
                            FREE()
                            pthread_exit()
                        yield()
                        pthread_join()
                        torchatproto_attach() <int torchatproto_attach (int s) at torchatproto.c:117>:
                            MALLOC()
                            torchatproto_hquery() <void *torchatproto_hquery (struct hvfs hvfs, const void *type) at torchatproto.c:151>:
                            close()
                            torchatproto_hclose() <void torchatproto_hclose (struct hvfs hvfs) at torchatproto.c:143>:
                                FREE()
                            torchatproto_hdone() <int torchatproto_hdone (struct hvfs hvfs, int64_t deadline) at torchatproto.c:92>
                            torchatproto_msendl() <int torchatproto_msendl (struct msock_vfs mvfs, struct iolist first, struct iolist last, int64_t deadline) at torchatproto.c:44>:
                                CONT()
                                fd_send()
                            torchatproto_mrecvl() <ssize_t torchatproto_mrecvl (struct msock_vfs mvfs, struct iolist first, struct iolist last, int64_t deadline) at torchatproto.c:59>:
                                CONT()
                                recv()
                                fcntl()
                                assert()
                                fdin()
                            hmake()
                            FREE()
                        torchatproto_msend() <ssize_t torchatproto_msend (int h, void *buf, size_t len, int64_t deadline) at torchatproto.c:323>:
                            hquery()
                            generate_dimension_head() <void generate_dimension_head (char *buf, size_t len) at torchatproto.c:312>:
                            memcpy()
                        strlen()
                        torchatproto_detach() <int torchatproto_detach (int h) at torchatproto.c:164>:
                            hquery()
                            FREE()
                            fdclean()
                        terminate_connection_with_domain() <void terminate_connection_with_domain (const int sock) at socks_helper.c:119>:
                            proxysocket_disconnect()
                            close()
                    FREE()
                    log_info()
                    torchatproto_msend() <ssize_t torchatproto_msend (int h, void *buf, size_t len, int64_t deadline) at torchatproto.c:323>:
                        hquery()
                        generate_dimension_head() <void generate_dimension_head (char *buf, size_t len) at torchatproto.c:312>:
                        memcpy()
                    strlen()
                    get_tor_error() <char *get_tor_error () at socks_helper.c:29>:
                        STRDUP()
                    log_err()
            client_update() <void client_update (struct data_wrapper data, int sock, int64_t deadline) at actions.c:150>:
                FREE()
                STRDUP()
                strlen()
                get_unread_message() <struct message get_unread_message (const char *peerId) at datastruct.c:133>
                convert_datastruct_to_char()
                torchatproto_msend() <ssize_t torchatproto_msend (int h, void *buf, size_t len, int64_t deadline) at torchatproto.c:323>:
                    hquery()
                    generate_dimension_head() <void generate_dimension_head (char *buf, size_t len) at torchatproto.c:312>:
                    memcpy()
            send_peer_list_to_client() <void send_peer_list_to_client (struct data_wrapper data, int sock, int64_t deadline) at actions.c:200>:
                FREE()
                get_peer_list() <char *get_peer_list () at datastruct.c:165>:
                    HASH_ITER()
                    strlen()
                    CALLOC()
                    strncat()
                STRDUP()
                convert_datastruct_to_char()
                torchatproto_msend() <ssize_t torchatproto_msend (int h, void *buf, size_t len, int64_t deadline) at torchatproto.c:323>:
                    hquery()
                    generate_dimension_head() <void generate_dimension_head (char *buf, size_t len) at torchatproto.c:312>:
                    memcpy()
                strlen()
            send_hostname_to_client() <void send_hostname_to_client (struct data_wrapper data, int sock, char *hostname, int64_t deadline) at actions.c:181>:
                FREE()
                STRDUP()
                convert_datastruct_to_char()
                torchatproto_msend() <ssize_t torchatproto_msend (int h, void *buf, size_t len, int64_t deadline) at torchatproto.c:323>:
                    hquery()
                    generate_dimension_head() <void generate_dimension_head (char *buf, size_t len) at torchatproto.c:312>:
                    memcpy()
                strlen()
            close()
            torchatproto_detach() <int torchatproto_detach (int h) at torchatproto.c:164>:
                hquery()
                FREE()
                fdclean()
            assert()
            FREE()
        FREE()
        shutdown()
    shutdown()
    close()
    clear_datastructs() <void clear_datastructs () at datastruct.c:188>:
        HASH_ITER()
        delete_peer() <void delete_peer (struct peer currentPeer) at datastruct.c:121>:
            HASH_DEL()
            FREE()
    log_clear_datastructs()
    FREE()
    destroy_proxy_connection() <void destroy_proxy_connection () at socks_helper.c:127>:
        proxysocketconfig_free()
    parse_config() <int parse_config (char *filename, char ***returnOpts) at parseconfig.c:94>:
        assert()
        fopen()
        MALLOC()
        getline()
        strip() <void strip (char *line, char del) at parseconfig.c:24>:
            strlen()
        strlen()
        sscanf()
        valid_opt() <bool valid_opt (char *opt) at parseconfig.c:86>:
            strlen()
        destroy_mat() <void destroy_mat (char **mat, int n) at parseconfig.c:76>:
            FREE()
        fclose()
        format_option() <void format_option (char *opt) at parseconfig.c:40>:
            strncpy()
            strncat()
            strlen()
        STRDUP()
        memset()
        FREE()
        create_opts_vector() <char **create_opts_vector (int size) at parseconfig.c:51>:
            MALLOC()
        fill_opts_vector() <void fill_opts_vector (char **retOpts, int n, char **opt, char **val, int pos) at parseconfig.c:60> (R):
            STRDUP()
            fill_opts_vector() <void fill_opts_vector (char **retOpts, int n, char **opt, char **val, int pos) at parseconfig.c:60> (recursive: see 254)
        printf()
    perror()
    OPT_GROUP()
    OPT_INTEGER()
    OPT_LONG()
    OPT_STRING()
    OPT_END()
    argparse_init()
    argparse_parse()
