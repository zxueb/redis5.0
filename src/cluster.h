#ifndef __CLUSTER_H
#define __CLUSTER_H

/*-----------------------------------------------------------------------------
 * Redis cluster data structures, defines, exported API.
 *----------------------------------------------------------------------------*/

#define CLUSTER_SLOTS 16384
#define CLUSTER_OK 0          /* Everything looks ok */ //表示集群正常运行
#define CLUSTER_FAIL 1        /* The cluster can't work */ //表示集群无法工作
#define CLUSTER_NAMELEN 40    /* sha1 hex length */ //Redis 集群节点名称的长度，以 SHA1 十六进制编码表示。默认值为 40。
#define CLUSTER_PORT_INCR 10000 /* Cluster port = baseport + PORT_INCR */ //集群节点的端口增量

/* The following defines are amount of time, sometimes expressed as
 * multiplicators of the node timeout value (when ending with MULT). */
#define CLUSTER_DEFAULT_NODE_TIMEOUT 15000 //集群节点的默认超时时间，以毫秒为单位。超过该时间节点未响应将被认为是不可达的。
#define CLUSTER_DEFAULT_SLAVE_VALIDITY 10 /* Slave max data age factor. */ //用于判断从节点数据是否有效的因子，默认值为 10。
#define CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE 1 //是否要求集群中的所有槽位都被分配到有效的节点，默认值为 1（开启）。如果启用，集群会在启动时检查槽位是否完全分配给节点，如果存在未分配的槽位，则集群无法正常工作。
#define CLUSTER_DEFAULT_SLAVE_NO_FAILOVER 0 /* Failover by default. */ //从节点是否禁止进行故障转移，默认值为 0（允许故障转移）。如果设置为 1，从节点将不会成为故障转移的目标。
#define CLUSTER_FAIL_REPORT_VALIDITY_MULT 2 /* Fail report validity. */ //故障报告的有效性乘数，默认值为 2。当节点被标记为故障时，会在 node timeout * FAIL_REPORT_VALIDITY_MULT 时间内保持故障状态。
#define CLUSTER_FAIL_UNDO_TIME_MULT 2 /* Undo fail if master is back. */ //当主节点恢复正常后，解除故障状态的时间乘数，默认值为 2。当主节点恢复后，会在 node timeout * FAIL_UNDO_TIME_MULT 时间后解除故障状态。
#define CLUSTER_FAIL_UNDO_TIME_ADD 10 /* Some additional time. */ //解除故障状态的附加时间，默认值为 10。当主节点恢复后，除了乘数所需的时间，还会额外等待 FAIL_UNDO_TIME_ADD 时间
#define CLUSTER_FAILOVER_DELAY 5 /* Seconds */ //故障转移的延迟时间，默认值为 5 秒。在进行故障转移之前，会等待一段时间，以确保主节点不会在短时间内自动恢复。
#define CLUSTER_DEFAULT_MIGRATION_BARRIER 1 //数据迁移的障碍值，默认值为 1。在进行数据迁移时，如果目标节点的槽位负载与源节点的负载之差超过障碍值，才会触发数据迁移。
#define CLUSTER_MF_TIMEOUT 5000 /* Milliseconds to do a manual failover. */ //手动故障转移的超时时间，默认值为 5000 毫秒（5 秒）。如果在超时时间内无法完成手动故障转移的操作，操作将被中断。
#define CLUSTER_MF_PAUSE_MULT 2 /* Master pause manual failover mult. */ //主节点暂停手动故障转移的乘数，默认值为 2。在进行手动故障转移时，如果主节点暂停了操作，则会等待 MF_PAUSE_MULT * node timeout 时间后继续操作。
#define CLUSTER_SLAVE_MIGRATION_DELAY 5000 /* Delay for slave migration. */ //从节点迁移的延迟时间，默认值为 5000 毫秒（5 秒）。在进行从节点迁移时，会等待一段时间，以确保从节点完成复制和同步操作。

/* Redirection errors returned by getNodeByQuery(). */
#define CLUSTER_REDIR_NONE 0          /* Node can serve the request. */ //表示节点可以直接处理请求，无需重定向。
#define CLUSTER_REDIR_CROSS_SLOT 1    /* -CROSSSLOT request. */ //表示请求涉及多个槽位（slots），需要进行跨槽重定向。
#define CLUSTER_REDIR_UNSTABLE 2      /* -TRYAGAIN redirection required */ //表示请求的目标节点处于不稳定状态，需要进行重试的重定向。
#define CLUSTER_REDIR_ASK 3           /* -ASK redirection required. */ //表示需要进行 ASK 重定向。当一个节点在处理请求时，发现目标槽位实际不在该节点上，但它仍然可以提供给定槽位的服务，并向客户端发送一个 ASK 响应，告知客户端将请求发送到正确的节点
#define CLUSTER_REDIR_MOVED 4         /* -MOVED redirection required. */ //表示需要进行 MOVED 重定向。当一个节点在处理请求时，发现目标槽位实际不在该节点上，并向客户端发送一个 MOVED 响应，告知客户端将请求发送到正确的节点。
#define CLUSTER_REDIR_DOWN_STATE 5    /* -CLUSTERDOWN, global state. */ //表示集群处于 CLUSTERDOWN 的全局状态，需要进行重定向。
#define CLUSTER_REDIR_DOWN_UNBOUND 6  /* -CLUSTERDOWN, unbound slot. */ //表示请求的槽位尚未分配给任何节点，需要进行重定向。

struct clusterNode;

/* clusterLink encapsulates everything needed to talk with a remote node. 封装与远程节点通信所需的所有信息*/
typedef struct clusterLink {
    mstime_t ctime;             /* Link creation time 链接创建时间，记录了创建链接的时间戳。*/
    int fd;                     /* TCP socket file descriptor */
    sds sndbuf;                 /* Packet send buffer  发送缓冲区，用于存储待发送的数据包*/
    sds rcvbuf;                 /* Packet reception buffer 接收缓冲区，用于存储接收到的数据包*/
    struct clusterNode *node;   /* Node related to this link if any, or NULL 与该链接相关联的节点，如果没有关联节点则为 NULL。*/
} clusterLink;

/* Cluster node flags and macros. */
#define CLUSTER_NODE_MASTER 1     /* The node is a master 表示节点是一个主节点*/
#define CLUSTER_NODE_SLAVE 2      /* The node is a slave 表示节点是一个从节点。*/
#define CLUSTER_NODE_PFAIL 4      /* Failure? Need acknowledge 表示节点处于可能失败的状态，并需要确认*/
#define CLUSTER_NODE_FAIL 8       /* The node is believed to be malfunctioning 表示节点被认为发生故障*/
#define CLUSTER_NODE_MYSELF 16    /* This node is myself 表示节点是当前节点自身*/
#define CLUSTER_NODE_HANDSHAKE 32 /* We have still to exchange the first ping 表示节点之间还未进行第一次握手*/
#define CLUSTER_NODE_NOADDR   64  /* We don't know the address of this node 表示无法获取该节点的地址信息*/
#define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node 表示向该节点发送 MEET 消息，用于节点间的相互发现*/
#define CLUSTER_NODE_MIGRATE_TO 256 /* Master elegible for replica migration. 表示主节点可作为从节点迁移的目标节点*/
#define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failver. 表示从节点不会尝试进行故障转移*/
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000" //于表示空节点或无效节点的名称。

#define nodeIsMaster(n) ((n)->flags & CLUSTER_NODE_MASTER)
#define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
#define nodeInHandshake(n) ((n)->flags & CLUSTER_NODE_HANDSHAKE)
#define nodeHasAddr(n) (!((n)->flags & CLUSTER_NODE_NOADDR))
#define nodeWithoutAddr(n) ((n)->flags & CLUSTER_NODE_NOADDR)
#define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
#define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)

/* Reasons why a slave is not able to failover. */
#define CLUSTER_CANT_FAILOVER_NONE 0  //没有任何原因导致从节点无法执行故障转移。
#define CLUSTER_CANT_FAILOVER_DATA_AGE 1 //从节点的数据过时，无法满足执行故障转移的要求。
#define CLUSTER_CANT_FAILOVER_WAITING_DELAY 2 //从节点正在等待一段延迟时间，在此期间无法执行故障转移。
#define CLUSTER_CANT_FAILOVER_EXPIRED 3 //从节点的故障转移状态已过期，无法继续执行故障转移。
#define CLUSTER_CANT_FAILOVER_WAITING_VOTES 4 //从节点正在等待其他节点的投票，在此期间无法执行故障转移。
#define CLUSTER_CANT_FAILOVER_RELOG_PERIOD (60*5) /* seconds. */ //从节点重新记录故障转移的周期时间，以秒为单位。如果在此周期内已经记录了故障转移，则无法继续执行故障转移

/* clusterState todo_before_sleep flags. */
#define CLUSTER_TODO_HANDLE_FAILOVER (1<<0) //表示在进入睡眠状态前需要处理故障转移。这可能涉及选举新的主节点或执行其他与故障转移相关的操作
#define CLUSTER_TODO_UPDATE_STATE (1<<1) //表示在进入睡眠状态前需要更新集群状态。这可能包括更新节点信息、更新故障转移状态等。
#define CLUSTER_TODO_SAVE_CONFIG (1<<2) //表示在进入睡眠状态前需要保存集群的配置信息。这通常涉及将配置写入磁盘，以便在重启后能够恢复相同的配置
#define CLUSTER_TODO_FSYNC_CONFIG (1<<3) //表示在进入睡眠状态前需要将集群的配置信息进行fsync操作，以确保数据持久化到磁盘。

/* Message types.
 *
 * Note that the PING, PONG and MEET messages are actually the same exact
 * kind of packet. PONG is the reply to ping, in the exact format as a PING,
 * while MEET is a special PING that forces the receiver to add the sender
 * as a node (if it is not already in the list). */
#define CLUSTERMSG_TYPE_PING 0          /* Ping Ping消息，用来向其他节点发送当前节点信息*/
#define CLUSTERMSG_TYPE_PONG 1          /* Pong (reply to Ping) Pong消息，对Ping消息的回复*/
#define CLUSTERMSG_TYPE_MEET 2          /* Meet "let's join" message Meet消息，表示某个节点要加入集群*/
#define CLUSTERMSG_TYPE_FAIL 3          /* Mark node xxx as failing Fail消息，表示某个节点有故障*/
#define CLUSTERMSG_TYPE_PUBLISH 4       /* Pub/Sub Publish propagation 用于在集群中传播发布订阅消息。*/
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST 5 /* May I failover? Failover授权请求消息，用于向其他节点请求进行故障转移。*/
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK 6     /* Yes, you have my vote Failover授权回复消息，表示其他节点同意进行故障转移。*/
#define CLUSTERMSG_TYPE_UPDATE 7        /* Another node slots configuration Slots配置更新消息，用于通知其他节点有关槽位分配的变更。*/
#define CLUSTERMSG_TYPE_MFSTART 8       /* Pause clients for manual failover Manual failover开始消息，用于通知节点在进行手动故障转移时暂停客户端请求。*/
#define CLUSTERMSG_TYPE_MODULE 9        /* Module cluster API message. 模块集群API消息，用于模块间的集群通信*/
#define CLUSTERMSG_TYPE_COUNT 10        /* Total number of message types. 消息类型的总数，用于表示所有消息类型的数量。*/

/* Flags that a module can set in order to prevent certain Redis Cluster
 * features to be enabled. Useful when implementing a different distributed
 * system on top of Redis Cluster message bus, using modules. */
#define CLUSTER_MODULE_FLAG_NONE 0  //无标志，表示没有设置任何特定的标志。
#define CLUSTER_MODULE_FLAG_NO_FAILOVER (1<<1) //禁止故障转移标志，表示模块不允许进行故障转移操作。当模块设置了此标志后，集群将不会在该模块上执行故障转移
#define CLUSTER_MODULE_FLAG_NO_REDIRECTION (1<<2) //禁止重定向标志，表示模块不允许进行重定向操作。当模块设置了此标志后，在执行相关操作时，集群将不会对请求进行重定向

/* This structure represent elements of node->fail_reports. 用于表示集群中节点的故障报告*/
typedef struct clusterNodeFailReport {
    struct clusterNode *node;  /* Node reporting the failure condition. */
    mstime_t time;             /* Time of the last report from this node. */
} clusterNodeFailReport;

typedef struct clusterNode {
    mstime_t ctime; /* Node object creation time. */
    char name[CLUSTER_NAMELEN]; /* Node name, hex string, sha1-size */
    int flags;      /* CLUSTER_NODE_... 节点的标志位，用于表示节点的状态和属性*/
    uint64_t configEpoch; /* Last configEpoch observed for this node 节点最后观察到的配置纪元（configEpoch）值*/
    unsigned char slots[CLUSTER_SLOTS/8]; /* slots handled by this node 位图中的每个位表示一个槽位，如果位为1，则表示该节点负责处理对应的槽位。*/
    int numslots;   /* Number of slots handled by this node 节点负责处理的槽位数。*/
    int numslaves;  /* Number of slave nodes, if this is a master 如果节点是主节点（master），则表示其拥有的从节点（slave）的数量*/
    struct clusterNode **slaves; /* pointers to slave nodes 指向从节点的指针数组，记录该主节点拥有的从节点信息*/
    struct clusterNode *slaveof; /* pointer to the master node. Note that it
                                    may be NULL even if the node is a slave
                                    if we don't have the master node in our
                                    tables. 指向主节点的指针，如果节点是从节点，则指示它所属的主节点。如果在当前节点的路由表中找不到主节点的信息，则该字段可能为NULL*/
    mstime_t ping_sent;      /* Unix time we sent latest ping 发送最近一次 ping 请求的时间戳。*/
    mstime_t pong_received;  /* Unix time we received the pong 接收到最近一次 pong 响应的时间戳*/
    mstime_t data_received;  /* Unix time we received any data 接收到最近一次数据的时间戳*/
    mstime_t fail_time;      /* Unix time when FAIL flag was set 节点被标记为失败（fail）的时间戳*/
    mstime_t voted_time;     /* Last time we voted for a slave of this master 最后一次为该主节点的从节点投票的时间戳*/
    mstime_t repl_offset_time;  /* Unix time we received offset for this node 收到针对该节点的复制偏移量（repl offset）的时间戳*/
    mstime_t orphaned_time;     /* Starting time of orphaned master condition 孤立主节点的起始时间戳*/
    long long repl_offset;      /* Last known repl offset for this node. 该节点的最新已知复制偏移量*/
    char ip[NET_IP_STR_LEN];  /* Latest known IP address of this node 该节点的最新已知 IP 地址。*/
    int port;                   /* Latest known clients port of this node 该节点的最新已知客户端端口。*/
    int cport;                  /* Latest known cluster port of this node. 该节点的最新已知集群通信端口。*/
    clusterLink *link;          /* TCP/IP link with this node 与该节点建立的 TCP/IP 连接*/
    list *fail_reports;         /* List of nodes signaling this as failing 记录了报告该节点故障的其他节点的信息。*/
} clusterNode;

typedef struct clusterState {
    clusterNode *myself;  /* This node */
    uint64_t currentEpoch; //当前集群的纪元（epoch）值
    int state;            /* CLUSTER_OK, CLUSTER_FAIL, ...集群的状态，可以是 CLUSTER_OK、CLUSTER_FAIL 等 */
    int size;             /* Num of master nodes with at least one slot 至少拥有一个槽位的主节点（master node）的数量*/
    dict *nodes;          /* Hash table of name -> clusterNode structures 存储节点名称（name）与 clusterNode 结构体的映射关系，用于快速查找节点信息。*/
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds.存储在一段时间内不再重新添加到集群中的节点的黑名单 */
    clusterNode *migrating_slots_to[CLUSTER_SLOTS]; //表示当前节点负责的 slot 正在迁往哪个节点。比如，migrating_slots_to[K] = node1，这就表示当前节点负责的 slot K，正在迁往 node1
    clusterNode *importing_slots_from[CLUSTER_SLOTS];//表示当前节点正在从哪个节点迁入某个 slot。比如，importing_slots_from[L] = node3，这就表示当前节点正从 node3 迁入 slot L
    clusterNode *slots[CLUSTER_SLOTS];//表示 16384 个 slot 分别是由哪个节点负责的。比如，slots[M] = node2，这就表示 slot M 是由 node2 负责的。
    uint64_t slots_keys_count[CLUSTER_SLOTS]; //记录每个槽位上的键数量
    rax *slots_to_keys; //用来记录 slot 和 key 的对应关系，可以通过它快速找到 slot 上有哪些 keys。
    /* The following fields are used to take the slave state on elections. */
    mstime_t failover_auth_time; /* Time of previous or next election. 上次或下次选举的时间戳*/
    int failover_auth_count;    /* Number of votes received so far. 迄今为止收到的选举票数。*/
    int failover_auth_sent;     /* True if we already asked for votes. 如果已经请求了选举票，则为真*/
    int failover_auth_rank;     /* This slave rank for current auth request. 当前授权请求的从节点排名*/
    uint64_t failover_auth_epoch; /* Epoch of the current election.当前选举的纪元 */
    int cant_failover_reason;   /* Why a slave is currently not able to
                                   failover. See the CANT_FAILOVER_* macros.表示从节点当前无法进行故障转移的原因，使用 CANT_FAILOVER_* 宏定义表示。 */
    /* Manual failover state in common. */
    mstime_t mf_end;            /* Manual failover time limit (ms unixtime).
                                   It is zero if there is no MF in progress. 手动故障转移的时间限制，以毫秒表示。*/
    /* Manual failover state of master. */
    clusterNode *mf_slave;      /* Slave performing the manual failover. 执行手动故障转移的从节点*/
    /* Manual failover state of slave. */
    long long mf_master_offset; /* Master offset the slave needs to start MF
                                   or zero if stil not received. 从节点需要开始手动故障转移的主节点偏移量*/
    int mf_can_start;           /* If non-zero signal that the manual failover
                                   can start requesting masters vote.如果非零，表示可以开始请求主节点投票进行手动故障转移 */
    /* The followign fields are used by masters to take state on elections. */
    uint64_t lastVoteEpoch;     /* Epoch of the last vote granted. 最后一次授予投票的纪元。*/
    int todo_before_sleep; /* Things to do in clusterBeforeSleep(). 在 clusterBeforeSleep() 函数中执行的任务*/
    /* Messages received and sent by type. */
    long long stats_bus_messages_sent[CLUSTERMSG_TYPE_COUNT]; //按类型统计发送的消息数量。
    long long stats_bus_messages_received[CLUSTERMSG_TYPE_COUNT]; //按类型统计接收的消息数量。
    long long stats_pfail_nodes;    /* Number of nodes in PFAIL status,
                                       excluding nodes without address. 处于 PFAIL 状态的节点数量，不包括没有地址的节点*/
} clusterState;

/* Redis cluster messages header */

/* Initially we don't know our "name", but we'll find it once we connect
 * to the first node, using the getsockname() function. Then we'll use this
 * address for all the next messages. */
typedef struct {
    char nodename[CLUSTER_NAMELEN]; //节点名称
    uint32_t ping_sent; //节点发送Ping的时间
    uint32_t pong_received; //节点收到Pong的时间
    char ip[NET_IP_STR_LEN];  /* IP address last time it was seen  节点IP*/
    uint16_t port;              /* base port last time it was seen 节点和客户端的通信端口*/
    uint16_t cport;             /* cluster port last time it was seen 节点用于集群通信的端口*/
    uint16_t flags;             /* node->flags copy 节点的标记*/
    uint32_t notused1;  //未用字段
} clusterMsgDataGossip;

typedef struct {
    char nodename[CLUSTER_NAMELEN];
} clusterMsgDataFail;

typedef struct {
    uint32_t channel_len; //消息通道（channel）的长度
    uint32_t message_len; //消息内容的长度
    unsigned char bulk_data[8]; /* 8 bytes just as placeholder. 一个 8 字节的数组，用作占位符，实际的消息数据将在后续的字节中进行存储。*/
} clusterMsgDataPublish;

typedef struct {
    uint64_t configEpoch; /* Config epoch of the specified instance. */
    char nodename[CLUSTER_NAMELEN]; /* Name of the slots owner. */
    unsigned char slots[CLUSTER_SLOTS/8]; /* Slots bitmap. */
} clusterMsgDataUpdate;

typedef struct {
    uint64_t module_id;     /* ID of the sender module. 发送者模块的 ID。模块在 Redis 中是一种扩展机制，它允许开发者编写自定义的功能插件。module_id 是一个唯一标识符，用于识别发送消息的模块。*/
    uint32_t len;           /* ID of the sender module. */
    uint8_t type;           /* Type from 0 to 255. 消息的类型*/
    unsigned char bulk_data[3]; /* 3 bytes just as placeholder. 存储消息的实际数据*/
} clusterMsgModule;

union clusterMsgData {
    /* PING, MEET and PONG //Ping、Pong和Meet消息类型对应的数据结构*/
    struct {
        /* Array of N clusterMsgDataGossip structures */
        clusterMsgDataGossip gossip[1];
    } ping;

    /* FAIL消息类型对应的数据结构*/
    struct {
        clusterMsgDataFail about;
    } fail;

    /* PUBLISH消息类型对应的数据结构 */
    struct {
        clusterMsgDataPublish msg;
    } publish;

    /* UPDATE消息类型对应的数据结构 */
    struct {
        clusterMsgDataUpdate nodecfg;
    } update;

    /* MODULE消息类型对应的数据结构 */
    struct {
        clusterMsgModule msg;
    } module;
};

#define CLUSTER_PROTO_VER 1 /* Cluster bus protocol version. */

typedef struct {
    char sig[4];        /* Signature "RCmb" (Redis Cluster message bus).消息的签名，固定为 "RCmb"，用于标识 Redis 集群消息总线。 */
    uint32_t totlen;    /* Total length of this message 消息的总长度，包括消息头部和消息体的长度*/
    uint16_t ver;       /* Protocol version, currently set to 1. 消息的协议版本，当前设置为 1。*/
    uint16_t port;      /* TCP base port number. */
    uint16_t type;      /* Message type 消息类型*/
    uint16_t count;     /* Only used for some kind of messages. 只在某些特定类型的消息中使用，表示一些相关信息的计*/
    uint64_t currentEpoch;  /* The epoch accordingly to the sending node. 发送节点当前的纪元（epoch）*/
    uint64_t configEpoch;   /* The config epoch if it's a master, or the last
                               epoch advertised by its master if it is a
                               slave. 如果发送节点是主节点，则为其配置纪元（config epoch），如果是从节点，则为其主节点广播的最后一个纪元*/
    uint64_t offset;    /* Master replication offset if node is a master or
                           processed replication offset if node is a slave.如果发送节点是主节点，则表示主节点的复制偏移量；如果是从节点，则表示从节点的复制偏移量。 */
    char sender[CLUSTER_NAMELEN]; /* Name of the sender node 发送消息节点的名称*/
    unsigned char myslots[CLUSTER_SLOTS/8]; //发送消息节点负责的slots
    char slaveof[CLUSTER_NAMELEN]; //如果发送节点是从节点，则指示它的主节点名称；如果是主节点，则保留为空字符串。
    char myip[NET_IP_STR_LEN];    /* Sender IP, if not all zeroed. 发送消息节点的IP*/
    char notused1[34];  /* 34 bytes reserved for future usage. */
    uint16_t cport;      /* Sender TCP cluster bus port 发送消息节点的通信端口*/
    uint16_t flags;      /* Sender node flags */
    unsigned char state; /* Cluster state from the POV of the sender */
    unsigned char mflags[3]; /* Message flags: CLUSTERMSG_FLAG[012]_... 消息的标志位，用于指示消息的特定属性，如是否需要 ACK、是否是故障转移相关等。 */
    union clusterMsgData data; //消息体
} clusterMsg;

#define CLUSTERMSG_MIN_LEN (sizeof(clusterMsg)-sizeof(union clusterMsgData))

/* Message flags better specify the packet content or are used to
 * provide some information about the node state. */
#define CLUSTERMSG_FLAG0_PAUSED (1<<0) /* Master paused for manual failover. */
#define CLUSTERMSG_FLAG0_FORCEACK (1<<1) /* Give ACK to AUTH_REQUEST even if
                                            master is up. */

/* ---------------------- API exported outside cluster.c -------------------- */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot, int *ask);
int clusterRedirectBlockedClientIfNeeded(client *c);
void clusterRedirectClient(client *c, clusterNode *n, int hashslot, int error_code);

#endif /* __CLUSTER_H */
