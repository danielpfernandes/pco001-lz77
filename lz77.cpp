// Copyright (c) 2021-11-7.
// Trabalho de PCO001 - Algoritmos e Estruturas de Dados UNIFEI
// Baseado no algoritmo de compressao proposto por Maxim Voloshin em
// https://codereview.stackexchange.com/questions/248158/lz77-compressor-and-decompressor-in-c
// OBS: Arquivo executavel criado para Windows com MingGW32, comando:
//      x86_64-w64-mingw32-c++.exe -o lz77.exe .\main.cpp
// Equipe: Alexandre L. Sousa, Daniel P. Fernandes, Natalia S. Sanchez

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

using namespace std;

unsigned int max_buf_length=0;
unsigned int max_dict_length=0;
unsigned int cur_dict_length=0;
unsigned int cur_buf_length=0;

struct link {
    unsigned short length;
    unsigned short offset;
    unsigned char next;
};

struct Node
{
    Node* prev;
    unsigned int index;
};

class ListaEncadeada
{
public: Node* lastNode;

    ListaEncadeada()
    {
        lastNode=nullptr;
    }

    ~ListaEncadeada()
    {
        Node* temp;
        while(lastNode!=nullptr)
        {
            temp=lastNode;
            lastNode = lastNode->prev;
            delete temp;
        }
    }

    void PushBack(unsigned int val)
    {
        Node* myNode = new Node;
        myNode->index=val;
        myNode->prev=lastNode;
        lastNode=myNode;
    }
};

/**
 * Le arquivo de entrada
 * @param raw arquivo em estado raw
 * @param inp entrada
 * @return tamanho do arquivo
 */
unsigned int leArquivo(unsigned char* &raw, fstream &inp)
{
    inp.seekg(0, ios::beg);
    unsigned int file_start = inp.tellg();
    inp.seekg(0, ios::end);
    unsigned int file_end = inp.tellg();
    unsigned int file_size = file_end - file_start;
    inp.seekg(0, ios::beg);
    raw = new unsigned char[file_size];
    inp.read((char*)raw, file_size);
    return file_size;
}

/**
 * Encontra a maior sequencia de simbolos existentes no buffer
 * @param s caractere
 * @param buf_start Inicio do buffer
 * @param len tamanho do link
 * @param off offset do link
 * @param dict Dicionario
 */
void EncontraPadraoMaisLongo(const unsigned char* s, unsigned int buf_start, unsigned short &len, unsigned short &off, ListaEncadeada dict[])
{
    Node* current = dict[s[buf_start]].lastNode;
    unsigned int max_offset = buf_start - cur_dict_length;
    unsigned int j;
    unsigned int k;
    if(current!=nullptr && (current->index)>=max_offset) { len=1; off=buf_start-(current->index); }
    while(current!=nullptr && (current->index)>=max_offset)
    {
        j=1;
        k=1;
        while(k<cur_buf_length && s[(current->index)+j]==s[buf_start+k])
        {
            if((current->index)+j==buf_start-1) { j=0; }
            else j++;
            k++;
        }
        if(k>len)
        {
            len = k;
            off = buf_start-(current->index);
            if(len==cur_buf_length) break;
        }
        else
        {
            current=current->prev;
        }
    }
}

/**
 * Atualiza o dicionario no processo de compressao de dados
 * @param s caractere a ser comprimido
 * @param shift_start marcacao de inicio da posicao janela no dicionario
 * @param Length tamanho a ser percorrido
 * @param dict dicionario
 * @return
 */
int AtualizaDicionario(const unsigned char* s, unsigned int shift_start, unsigned short Length, ListaEncadeada dict[])
{
    for(unsigned int i=shift_start; i<(shift_start+Length+1); i++)
    {
        dict[s[i]].PushBack(i);
    }
    return Length;
}

/**
 * Compacta conjunto de dados e cria ligacao no dicionario
 * @param inp
 * @param out
 */
void compactaEEscreveLink(link inp, vector<unsigned char> &out)
{
    if(inp.length!=0)
    {
        out.push_back((unsigned char)((inp.length << 4) | (inp.offset >> 8)));
        out.push_back((unsigned char)(inp.offset));
    }
    else
    {
        out.push_back((unsigned char)((inp.length << 4)));
    }
    out.push_back(inp.next);
}

/**
 * Comprime dado de entrada
 * @param file_size Tamanho do arquivo
 * @param data Dado de entrada
 * @param file_out Saida comprimida em tipo fstream
 */
void comprimeDado(unsigned int file_size, unsigned char* data, fstream &file_out)
{
    ListaEncadeada dict[256];
    link myLink{};
    vector<unsigned char> lz77_coded;
    lz77_coded.reserve(2*file_size);
    bool hasOddSymbol=false;
    unsigned int target_size;
    file_out.seekp(0, ios_base::beg);
    cur_dict_length = 0;
    cur_buf_length = max_buf_length;
    for(unsigned int i=0; i<file_size; i++)
    {
        if((i+max_buf_length)>=file_size)
        {
            cur_buf_length = file_size-i;
        }
        myLink.length=0;myLink.offset=0;
        EncontraPadraoMaisLongo(data, i, myLink.length, myLink.offset, dict);
        i= i + AtualizaDicionario(data, i, myLink.length, dict);
        if(i<file_size) {
            myLink.next=data[i]; }
        else { myLink.next='_'; hasOddSymbol=true; }
        compactaEEscreveLink(myLink, lz77_coded);
        if(cur_dict_length<max_dict_length) {
            while((cur_dict_length < max_dict_length) && cur_dict_length < (i+1)) cur_dict_length++;
        }
    }
    if(hasOddSymbol) { lz77_coded.push_back((unsigned char)0x1); }
    else lz77_coded.push_back((unsigned char)0x0);
    target_size=lz77_coded.size();
    file_out.write((char*)lz77_coded.data(), target_size);
    lz77_coded.clear();
    printf("Tamanho do arquivo de saida: %d bytes\n", target_size);
    printf("Taxa de compressao: %.3f:1\n", ((double)file_size/(double)target_size));
}

/**
 * Descomprime o dado do arquivo
 * @param file_size tamanho do arquivo
 * @param data dado comprimido
 * @param file_out saida descomprimida do tipo fstream
 */
void descomprimeDado(unsigned int file_size, const unsigned char* data, fstream &file_out)
{
    if(file_size==0) { printf("Erro! Arquivo de entrada vazia\n"); return; }
    link myLink{};
    vector<unsigned char> lz77_decoded;
    lz77_decoded.reserve(4*file_size);
    unsigned int target_size;
    unsigned int i=0;
    int k;
    file_out.seekg(0, ios::beg);
    while(i<(file_size-1))
    {
        if(i!=0) { lz77_decoded.push_back(myLink.next); }
        myLink.length = (unsigned short)(data[i] >> 4);
        if(myLink.length!=0)
        {
            myLink.offset = (unsigned short)(data[i] & 0xF);
            myLink.offset = myLink.offset << 8;
            myLink.offset = myLink.offset | (unsigned short)data[i+1];
            myLink.next = (unsigned char)data[i+2];
            if(myLink.offset>lz77_decoded.size()) {
                printf("Erro! Offset fora do alcance!");
                lz77_decoded.clear();
                return;
            }
            if(myLink.length>myLink.offset)
            {
                k = myLink.length;
                while(k>0)
                {
                    if(k>=myLink.offset)
                    {
                        lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end());
                        k=k-myLink.offset;
                    }
                    else
                    {
                        lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end()-myLink.offset+k);
                        k=0;
                    }
                }
            }
            else lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end()-myLink.offset+myLink.length);
            i++;
        }
        else {
            myLink.offset = 0;
            myLink.next = (unsigned char)data[i+1];
        }
        i=i+2;
    }
    unsigned char hasOddSymbol = data[file_size-1];
    if(hasOddSymbol==0x0 && file_size>1) { lz77_decoded.push_back(myLink.next); }
    target_size = lz77_decoded.size();
    file_out.write((char*)lz77_decoded.data(), target_size);
    lz77_decoded.clear();
    printf("Tamanho do arquivo de saida: %d bytes\n", target_size);
    printf("Descompressao concluida!");
}

/**
 * Classe principal da solucao
 * IMPORTANTE: Utilize o arquivo executavel
 *      (nos criamos para versao Windows com MingGW32 com comando: x86_64-w64-mingw32-c++.exe -o lz77.exe .\lz77.cpp)
 * @param argc comando executavel
 * @param argv argumentos: [-c | -u] [arquivo_de_entrada] [arquivo_de_saida]
 * @return 0 se bem-sucedido.
 */
int main(int argc, char* argv[])
{
    max_buf_length=15; //16-1;
    max_dict_length=4095; //4096-1
    string filename_in;
    string filename_out;
    fstream file_in;
    fstream file_out;
    unsigned int raw_size = 0;
    unsigned char* raw = nullptr;
    int mode;
    printf("Algoritmo LZ77 de compressao/descompressao\n");
    printf("PCO001 - baseado em codigo de MVoloshin, TSU, 2020\n");
    if(argc==4) {
        if(strcmp(argv[1], "-u")==0) mode = 0;
        else if(strcmp(argv[1], "-c")==0) mode = 1;
        else { printf("Parametro desconhecido, use -c ou -u\n"); return 0; }
        filename_in=std::string(argv[2]);
        filename_out=std::string(argv[3]);
    }
    else
    {
        printf("Uso: [-c | -u] [arquivo_de_entrada] [arquivo_de_saida]\n");
        printf("     Exemplo de compressao: lz77.exe -c teste.txt teste.zip\n");
        printf("     Exemplo de descompressao: lz77.exe -u teste.zip descomprimido.txt\n");
        return 0;
    }
    file_in.open(filename_in, ios::in | ios::binary);
    file_in.unsetf(ios_base::skipws);
    file_out.open(filename_out, ios::out);
    file_out.close();
    file_out.open(filename_out, ios::in | ios::out | ios::binary);
    file_out.unsetf(ios_base::skipws);
    if(file_in && file_out) {
        raw_size= leArquivo(raw, file_in);
        printf("Input file size: %d bytes\n", raw_size);
    }
    else
    {
        if(!file_in) printf("Erro! Nao foi possivel abrir o arquivo de entrada!\n");
        if(!file_out) printf("Error! Nao foi possivel abrir o arquivo de saida!\n");
        mode = -1;
    }
    file_in.close();
    if(mode==0)
    {
        descomprimeDado(raw_size, raw, file_out);
    }
    else if(mode==1)
    {
        comprimeDado(raw_size, raw, file_out);
    }
    delete[] raw;
    file_out.close();
    return 0;
}