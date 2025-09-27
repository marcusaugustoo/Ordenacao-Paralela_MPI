#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_SEQ_LENGTH 102
typedef char sequence[MAX_SEQ_LENGTH];

int compare_dna(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

sequence* read_file(const char *filename, int *n) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erro ao abrir input");
        exit(1);
    }
    int capacity = 1000;
    sequence *data = malloc(capacity * sizeof(sequence));
    char buf[MAX_SEQ_LENGTH];
    *n = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (*n >= capacity) {
            capacity *= 2;
            data = realloc(data, capacity * sizeof(sequence));
        }
        strcpy(data[(*n)++], buf);
    }
    fclose(fp);
    return data;
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) printf("Uso: mpirun -np N %s input.txt output.txt\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int global_n = 0;
    sequence *all_data = NULL;

    if (rank == 0) {
        all_data = read_file(argv[1], &global_n);
    }

    // Distribui quantidade de elementos
    MPI_Bcast(&global_n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int base = global_n / size;
    int resto = global_n % size;
    int local_n = (rank < resto) ? base + 1 : base;

    int *sendcounts = NULL, *displs = NULL;
    int *sendcounts_chars = NULL, *displs_chars = NULL;

    if (rank == 0) {
        sendcounts = malloc(size * sizeof(int));
        displs = malloc(size * sizeof(int));
        sendcounts_chars = malloc(size * sizeof(int));
        displs_chars = malloc(size * sizeof(int));
        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = (i < resto) ? base + 1 : base;
            displs[i] = offset;
            offset += sendcounts[i];
            sendcounts_chars[i] = sendcounts[i] * MAX_SEQ_LENGTH;
            displs_chars[i] = displs[i] * MAX_SEQ_LENGTH;
        }
    }

    sequence *local_data = malloc(local_n * sizeof(sequence));

    // Distribui os dados
    MPI_Scatterv(all_data, sendcounts_chars, displs_chars, MPI_CHAR,
                 local_data, local_n * MAX_SEQ_LENGTH, MPI_CHAR,
                 0, MPI_COMM_WORLD);

    double start = MPI_Wtime();

    // Passo 1: ordenação local
    qsort(local_data, local_n, sizeof(sequence), compare_dna);

    // Passo 2: escolher samples
    int s = size - 1;  // nº de pivôs
    sequence *samples = malloc(s * sizeof(sequence));
    for (int i = 0; i < s; i++) {
        int idx = (i+1) * local_n / (s+1);
        strcpy(samples[i], local_data[idx]);
    }

    // Coletar todos samples no root
    sequence *all_samples = NULL;
    if (rank == 0) all_samples = malloc(s * size * sizeof(sequence));
    MPI_Gather(samples, s * MAX_SEQ_LENGTH, MPI_CHAR,
               all_samples, s * MAX_SEQ_LENGTH, MPI_CHAR,
               0, MPI_COMM_WORLD);

    // Passo 3: root escolhe pivôs globais
    sequence *pivots = malloc((size-1) * sizeof(sequence));
    if (rank == 0) {
        qsort(all_samples, s * size, sizeof(sequence), compare_dna);
        for (int i = 0; i < size-1; i++) {
            strcpy(pivots[i], all_samples[(i+1)*s]);
        }
    }
    MPI_Bcast(pivots, (size-1) * MAX_SEQ_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Passo 4: cada processo divide os dados em buckets
    int *send_counts = calloc(size, sizeof(int));
    for (int i = 0; i < local_n; i++) {
        int bucket = 0;
        while (bucket < size-1 && strcmp(local_data[i], pivots[bucket]) > 0) bucket++;
        send_counts[bucket]++;
    }

    int *send_displs = malloc(size * sizeof(int));
    send_displs[0] = 0;
    for (int i = 1; i < size; i++) send_displs[i] = send_displs[i-1] + send_counts[i-1];

    sequence *send_buf = malloc(local_n * sizeof(sequence));
    int *bucket_pos = calloc(size, sizeof(int));
    for (int i = 0; i < local_n; i++) {
        int bucket = 0;
        while (bucket < size-1 && strcmp(local_data[i], pivots[bucket]) > 0) bucket++;
        strcpy(send_buf[send_displs[bucket] + bucket_pos[bucket]++], local_data[i]);
    }

    // Passo 5: All-toa-ll para saber quantos cada processo vai receber
    int *recv_counts = calloc(size, sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int *recv_displs = malloc(size * sizeof(int));
    recv_displs[0] = 0;
    for (int i = 1; i < size; i++) recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
    int recv_total = recv_displs[size-1] + recv_counts[size-1];

    sequence *recv_buf = malloc(recv_total * sizeof(sequence));

    int *send_counts_chars2 = malloc(size * sizeof(int));
    int *send_displs_chars2 = malloc(size * sizeof(int));
    int *recv_counts_chars2 = malloc(size * sizeof(int));
    int *recv_displs_chars2 = malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        send_counts_chars2[i] = send_counts[i] * MAX_SEQ_LENGTH;
        send_displs_chars2[i] = send_displs[i] * MAX_SEQ_LENGTH;
        recv_counts_chars2[i] = recv_counts[i] * MAX_SEQ_LENGTH;
        recv_displs_chars2[i] = recv_displs[i] * MAX_SEQ_LENGTH;
    }

    // Passo 6: All-to-all dos dados
    MPI_Alltoallv(send_buf, send_counts_chars2, send_displs_chars2, MPI_CHAR,
                  recv_buf, recv_counts_chars2, recv_displs_chars2, MPI_CHAR,
                  MPI_COMM_WORLD);

    // Ordenação final local
    qsort(recv_buf, recv_total, sizeof(sequence), compare_dna);

    double end = MPI_Wtime();
    double local_time = end - start;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    int *final_counts = NULL, *final_displs = NULL;
    if (rank == 0) {
        final_counts = malloc(size * sizeof(int));
        final_displs = malloc(size * sizeof(int));
    }
    int recv_total_chars = recv_total * MAX_SEQ_LENGTH;
    MPI_Gather(&recv_total_chars, 1, MPI_INT, final_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        final_displs[0] = 0;
        for (int i = 1; i < size; i++) final_displs[i] = final_displs[i-1] + final_counts[i-1];
        all_data = realloc(all_data, global_n * sizeof(sequence));
    }

    MPI_Gatherv(recv_buf, recv_total_chars, MPI_CHAR,
                all_data, final_counts, final_displs, MPI_CHAR,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        FILE *output = fopen(argv[2], "w");
        for (int i = 0; i < global_n; i++) fprintf(output, "%s\n", all_data[i]);
        fclose(output);
        printf("SampleSort concluido em %.6f segundos.\n", max_time);
    }

    MPI_Finalize();
    return 0;
}
