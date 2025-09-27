#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_SEQ_LENGTH 102  
#define DNA_CHARS "ACGT"

char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return s;
}

int compare_dna(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void sequential_sort(char** data, int n) {
    qsort(data, n, sizeof(char*), compare_dna);
}

void generate_dna_file(const char* filename, int num_sequences, int seq_length) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo para geração");
        exit(1);
    }
    srand(time(NULL));
    for (int i = 0; i < num_sequences; i++) {
        char seq[MAX_SEQ_LENGTH];
        for (int j = 0; j < seq_length; j++) {
            seq[j] = DNA_CHARS[rand() % 4];
        }
        seq[seq_length] = '\0';
        fprintf(fp, "%s\n", seq);
    }
    fclose(fp);
    printf("Gerado arquivo %s com %d sequências de comprimento %d.\n", filename, num_sequences, seq_length);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Uso: %s input.txt output.txt\n", argv[0]);
        return 1;
    }

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        perror("Erro ao abrir input");
        return 1;
    }

    int n = 0;
    char buf[MAX_SEQ_LENGTH];
    while (fgets(buf, sizeof(buf), input)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        trim(buf);
        if (strlen(buf) > 0) n++;
    }
    rewind(input);

    char** data = malloc(n * sizeof(char*));
    if (!data) {
        perror("Erro de alocação");
        return 1;
    }

    int i = 0;
    while (fgets(buf, sizeof(buf), input)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        trim(buf);
        len = strlen(buf);
        if (len == 0) continue;
        data[i] = strdup(buf);
        i++;
    }
    fclose(input);

    clock_t start = clock();
    sequential_sort(data, n);
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    FILE* output = fopen(argv[2], "w");
    if (!output) {
        perror("Erro ao abrir output");
        return 1;
    }

    for (i = 0; i < n; i++) {
        fprintf(output, "%s\n", data[i]);
        free(data[i]);
    }
    fclose(output);
    free(data);

    printf("SequencialSort concluido em %.6f segundos.\n", time_spent);

    return 0;
}