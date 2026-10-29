/**
 * Gera um header C com o conteúdo de um arquivo binário, no mesmo formato
 * do "xxd -i": const unsigned char NOME[] e const unsigned int NOME_len
 *
 * uso: bin2c <arquivo> <nome> > saida.h
 */
#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s <file> <name>\n", argv[0]);
		return 1;
	}
	FILE * file = fopen(argv[1], "rb");
	if(file == NULL)
	{
		perror(argv[1]);
		return 1;
	}
	unsigned int size = 0;
	int c;
	printf("const unsigned char %s[] = {", argv[2]);
	while((c = fgetc(file)) != EOF)
	{
		printf("%s0x%02x,", size % 12 == 0 ? "\n\t" : " ", c);
		size++;
	}
	printf("\n};\nconst unsigned int %s_len = %u;\n", argv[2], size);
	fclose(file);
	return 0;
}
