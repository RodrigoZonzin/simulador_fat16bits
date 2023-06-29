/*INCLUDE*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>

/*DEFINE*/
#define SECTOR_SIZE	     512
#define CLUSTER_SIZE     2*SECTOR_SIZE
#define ENTRY_BY_CLUSTER CLUSTER_SIZE/sizeof(dir_entry_t)
#define NUM_CLUSTER	     4096
#define fat_name	     "fat.part"

struct _dir_entry_t {
	char filename[18];
	uint8_t attributes;
	uint8_t reserved[7];
	uint16_t first_block;
	uint32_t size;
};

typedef struct _dir_entry_t  dir_entry_t;

union _data_cluster {
	dir_entry_t dir[ENTRY_BY_CLUSTER];
	uint8_t data[CLUSTER_SIZE];
};

typedef union _data_cluster data_cluster;

/*DATA DECLARATION*/
static uint16_t fat[NUM_CLUSTER];
static uint8_t boot_block[CLUSTER_SIZE];
static dir_entry_t root_dir[ENTRY_BY_CLUSTER];
static data_cluster clusters[4086];
static int current_rootdir_position = 0;
static int current_cluster_position = 0;

void init(void) {
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name,"wb");
	for (i = 0; i < CLUSTER_SIZE; ++i)
		boot_block[i] = 0xbb;

	fwrite(&boot_block, sizeof(boot_block), 1,ptr_file);

	fat[0] = 0xfffd;
	for (i = 1; i < 9; ++i)
		fat[i] = 0xfffe;

	fat[9] = 0xffff;
	for (i = 10; i < NUM_CLUSTER; ++i)
		fat[i] = 0x0000;

	fwrite(&fat, sizeof(fat), 1, ptr_file);

    memset(root_dir, 0, sizeof(root_dir));
    fwrite(&root_dir, sizeof(root_dir), 1,ptr_file);

    memset(clusters, 0, sizeof(clusters));
	fwrite(&clusters, sizeof(clusters), 1, ptr_file);

	fclose(ptr_file);
}

void load() {
	FILE* ptr_file;
	int i;
	ptr_file = fopen(fat_name, "rb");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);
	fread(fat, sizeof(fat), 1, ptr_file);
	fread(root_dir, sizeof(root_dir), 1, ptr_file);
	fclose(ptr_file);
}

void mkdir(char *dir_name){
	FILE* ptr_file;

	int i;
	ptr_file = fopen(fat_name, "r+"); 			 		//abre o arquivo para leitura e escrita
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);		//posiciona stream apos o bootblock
	fread(fat, sizeof(fat), 1, ptr_file);				//le o fat
	fread(root_dir, sizeof(root_dir), 1, ptr_file);		//le o root dir
	
	dir_entry_t new_dir = {dir_name, 1, {-1,-1,-1,-1,-1,-1,-1}, 0, 1};
	root_dir[current_rootdir_position] = new_dir;

	
	fclose(ptr_file);
	current_rootdir_position++;
}

void ls_root(){
	FILE *ptr_file; 
	int i; 

	ptr_file = fopen(fat_name, "wb");
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);		//posiciona stream apos o bootblock
	fread(fat, sizeof(fat), 1, ptr_file);				//le o fat
	fread(root_dir, sizeof(root_dir), 1, ptr_file);		//le o root dir

	for(int i = 0; i<ENTRY_BY_CLUSTER; i++){
		printf("Name: %s\n", root_dir[i].filename); 
		printf("Size: %d\n", root_dir[i].size); 
		printf("Attribute: %d\n\n", root_dir[i].attributes);
	}
}

void create(char *filename){
	FILE *ptr_file; 

	ptr_file = fopen(fat_name, "r+"); 			 		//abre o arquivo para leitura e escrita
	fseek(ptr_file, sizeof(boot_block), SEEK_SET);		//posiciona stream apos o bootblock
	fread(fat, sizeof(fat), 1, ptr_file);				//le o fat
	fread(root_dir, sizeof(root_dir), 1, ptr_file);		//le o diretório root
	fread(clusters, sizeof(clusters), 1, ptr_file); 	//le os data clusters

	dir_entry_t new_file = {filename, 0, {-1,-1,-1,-1,-1,-1,-1}, 0x001, 0}; 
	clusters->dir[current_cluster_position] = new_file; 

	fwrite(&fat, sizeof(fat), 1, ptr_file);				//salva no disco
	fwrite(&root_dir, sizeof(root_dir), 1, ptr_file);	//salva no disco
	fwrite(&clusters, sizeof(clusters), 1, ptr_file);	//salva no disco

	fclose(ptr_file);
	current_cluster_position++;
}

int main(int argc, char **argv){
	init();
	int escolha = -1; 
	char *nome_diretorio = (char*)malloc(18); 
	char *nome_arquivo = (char*)malloc(18);

	while(escolha != 0){
		printf("1-Criar um diretório na pasta raiz\n2-Listar diretorio\n3-Criar um arquivo\n0-sair\n");
		scanf("%d", &escolha);

		if(escolha == 1){
			printf("Qual o nome do diretorio? \n");
			scanf("%s", nome_diretorio);
			mkdir(nome_diretorio); 
		}

		if(escolha == 2){
			ls_root(); 
		}
		
		if(escolha == 3){
			printf("Digite o nome do arquivo: \n");
			scanf("%s", nome_arquivo);
			create(nome_arquivo); 
		}
	}

	return 0; 
}
