# Ordenação Paralela com MPI - Windows

Este projeto implementa algoritmos de ordenação **sequencial** e **paralela** usando MPI em C no Windows. 
No caso, foi utilizado o Sample Sort para testes comparativos.

## Estrutura do projeto


- sample/      Algoritmo + Resultados de execução paralela

- sequencial/  Algoritmo + Resultados de execução sequencial


> **Inputs não estão no repositório** devido ao tamanho (~2GB).  
> Use os arquivos de resultados para referência e faça download dos inputs do Drive.

---

## Pré-requisitos

- Microsoft MPI (MS-MPI) instalado
- Compilador GCC ou MinGW
- Windows 10/11

---

## Compilação

### Paralelo (MPI)

gcc sample.c -o sample.exe -I"C:\Caminho\Para\MSMPI\Include" -L"C:\Caminho\Para\MSMPI\Lib\x64" -lmsmpi

### Sequencial

gcc -o sequencial sequencial.c

> Substitua os caminhos pelo diretório correto do SDK do MPI no seu PC.

---

## Execução

### Paralelo (MPI)

# Executando com 2 processos
mpiexec -n 2 ./sample.exe input_100k.txt output_100k.txt

# Executando com 4 processos
mpiexec -n 4 ./sample.exe input_100k.txt output_100k.txt

# Executando com 8 processos
mpiexec -n 8 ./sample.exe input_100k.txt output_100k.txt

### Sequencial

# Executando o programa sequencial
./sequencial input_100k.txt output_100k.txt

---

## Inputs grandes

- Os arquivos de entrada (~2GB) **não estão no GitHub**.  
- Faça o download dos inputs do Drive: [Clique aqui para acessar](https://drive.google.com/drive/folders/1OqrW74QK4b7j7FShxUuRZcGjnP4uzhAG?usp=sharing)  
- Coloque os inputs na pasta `inputs/` local antes de executar os programas.

---


