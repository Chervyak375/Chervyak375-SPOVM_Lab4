#include <iostream>
#include <list>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <locale>
#include <codecvt>
#include <chrono>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/input.h>
#include <curses.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <algorithm>

pthread_mutex_t ready;
std::list<pthread_t> ids;
std::list<pthread_mutex_t> mutexs;
int sole = 0;
bool isPause=false;

char* getRandomString(const int len)
{
    std::time_t result = std::time(nullptr);
    std::localtime(&result);

    srand(result);
    char* s = (char*)malloc(len * sizeof(char));
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < (len-1); ++i)
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

    s[(len-1)] = alphanum[sole];

    if (sole >= len)
        sole = 0;

    sole++;
    s[len] = 0;

    return s;
}

std::wstring stringToWString(std::string string)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(string);

    return wide;
}

void *io(void* args)
{
    while(true)
    {
        if(ids.size()>0 && !isPause)
        {
            int index=0;
            for(pthread_t id: ids)
            {
                pthread_mutex_t* mutex;

                std::list<pthread_mutex_t>::iterator itM = mutexs.begin();
                std::advance(itM, index);
                mutex=&*itM;

                pthread_mutex_unlock(mutex);
                pthread_mutex_lock(&ready);

                index++;
            }

            printw("\n");
            refresh();
        }

        sleep(1);
    }

    pthread_exit(0);
}

void *potok(void* args)
{
    int index = *(int*)args;
    char* str = getRandomString(10);
    pthread_t id;
    pthread_mutex_t* mutex;

    std::list<pthread_t>::iterator itP = ids.begin();
	std::advance(itP, index);
	id=*itP;

    std::list<pthread_mutex_t>::iterator itM = mutexs.begin();
    std::advance(itM, index);
    mutex=&*itM;

    while(true)
    {
        pthread_mutex_lock(mutex);

        if(!(std::find(ids.begin(), ids.end(), id) != ids.end()))
        {
            pthread_mutex_unlock(mutex);
            pthread_exit(0);
        }

        for(int i=0; i<strlen(str); i++)
            printw("%c", str[i]);
        printw("\n");
        refresh();

        pthread_mutex_unlock(&ready);
    }
}

int main(int argc, char* argv[])
{
    initscr();
    refresh();

    printw("Control\n");
    printw("\t+ -- Add new process.\n");
    printw("\t- -- Delete last process.\n");
    printw("\tQ -- Quit.\n");
    printw("\tP -- Turn on/off pause.\n");
    printw("\n");
    refresh();

    pthread_mutex_init(&ready, NULL);
    pthread_mutex_lock(&ready);

    int ch;
    pthread_t thread;
    int rc = pthread_create(&thread, NULL, io, NULL);

    while (true)
    {
        ch = getch();
        refresh();
        ch = toupper(ch);

        switch (ch)
        {
        case '+':
        {
            printw("\n");
            refresh();


            pthread_mutex_t mutex;
            pthread_mutex_init(&mutex, NULL);
            pthread_mutex_lock(&mutex);
            mutexs.push_back(mutex);

            int index=ids.size();

            pthread_t id;
            pthread_create(&id, NULL, potok, &index);

            ids.push_back(id);
            sole=+ids.size();

            break;
        }
        case '-':
        {
            printw("\n");
            refresh();

            if(ids.size()>0)
            {
                ids.remove(ids.back());
                pthread_mutex_destroy(&mutexs.back());
                mutexs.pop_back();
            }

            break;
        }
        case 'Q':
        {
            printw("\n");
            refresh();

            while(!ids.empty())
            {
                ids.remove(ids.back());
                pthread_mutex_destroy(&mutexs.back());
                mutexs.pop_back();
            }

            return 0;
        }
        case 'P':
        {
            isPause=!isPause;

            printw("\n");
            refresh();
        }
        default:
        {
            break;
        }
        }
    }

    return 0;
}
