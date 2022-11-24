typedef u16 short;
typedef byte char;
typedef fspoint int;
typedef inode int;
// 200MiB

struct _superblock{
    size_t inode_amount;
    size_t data_blocks_amount; //6250
    size_t block_size; //32kB 
    int inode_start; //Forma de saber donde empiezan los inodos -> Si Inodo ~64b -> 512 inodos por bloque -> 13 bloques de inodos
    int data_start; 
}

//Serializar
// 2) Se puede asumir siempre mismo superbloque? -> Toda la info 
// Por cada inodo -> Serializar info del inodo -> Por cada bloque, guardar el binario hasta llegar a size.
// Guardar numero next_free
// Deserializado
// Cargar superbloque??
// Por cada inodo -> Cargarlo en su lugar -> Llegando a la data, guardar en nuevos bloques cada bloque

byte data_bmp[DATA_BLOCKS_AMOUNT] = {0};
byte inode_bmd[DATA_INODE_AMOUNT] = {0};

byte blocks[200MiB] = {0};

#define KiB 1024
#define BLOCK_SIZE 32*KiB

#define TO_N_BLOCK (n) blocks * (BLOCK_SIZE * n)

init(){
    struct _superblock* a = (struct superblock*)blocks;
    init_superblock(a);
    char *bmp = TO_N_BLOCK(blocks, n);
}


size_t root_no; //Alguna forma de guardar el root. 

struct _inode{

    // Permissions and Ownership
    u16 type_mode; //File/directory/links/
    int user_id;
    int group_id;

    // Time info
    int last_accessed;
    int created;
    int last_modified;
    int deleted;

    // Inode data

    size_t size;
    int link_counts;
    int block_count;
    fspoint block[INODE_BLOCK]; //12 directos, 1 indirecto 
     |
     |-> struct _dentry *directories;
    
    inode next_free;
}

inode_t free_inode = algo;

nuevo->next_free = free_inode;
free_inode = nuevo;

free_inode = next_free;


struct _dentry{
    inode inode_number;
    // int ent_size;
    // int strlen;
    char file_type;
    char file_name[MAX_FILE_NAME];
}

// Preguntas
// 1) Como se sabe cual es el ultimo dentry

// Link utiles
// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout