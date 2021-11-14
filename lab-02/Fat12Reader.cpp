#include <iostream>
#include <vector>
#include <array>
#include <stack>
#include <queue>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace std;

#define RED "\033[31m"  /* Red */
#define RESET "\033[0m" /* Reset */

#define DIR 0x10
#define FL 0x20
#define HDFL 0x27

typedef unsigned char u8;  //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32; //4字节

// 软盘中结构划分
const int BootStart = 0;
const int FAT1Start = 0x200;
const int FAT2Start = 0x1400;
const int RootDirectoryStart = 0x2600;
const int DataStart = 0x4200;
// 软盘大小
// wc -c $IMG
const int N = 1474560;
// 对IMG文件做映射
u8 IMG[N];

struct DirOrFileEntry {
    char DIR_Name[11]; // 文件名8字节 扩展名3字节
    u8 DIR_Attr;     // 文件属性
    char reserved[10]; // 保留位
    u16 DIR_WrtTime;  // 最后一次写入时间
    u16 DIR_WrtDate;  // 最后一次写入日期
    u16 DIR_FstClus;  // 此条目对应的开始簇号
    u32 DIR_FileSize; // 文件大小
    vector<struct DirOrFileEntry *> subDir;
};
static DirOrFileEntry rootDir;


extern "C" {
void PRINT_RED(const char *);
void PRINT_WHITE(const char *);
}

bool loadIMG(const char *img) {
    FILE *file = fopen(img, "r");
    bool ret = fread(IMG, sizeof(char), N, file) > 0;
    fclose(file);
    return ret;
}

vector<string> split(const string &s, const std::string &delim = " ") {
    vector<string> tokens;
    string::size_type lastPos = s.find_first_not_of(delim, 0);
    string::size_type pos = s.find_first_of(delim, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delim, pos);
        pos = s.find_first_of(delim, lastPos);
    }
    return tokens;
}

bool validPath(const char *path) {
    for (int i = 1; i < strlen(path); i++) {
        if (path[i] == '/') {
            if (path[i - 1] == path[i])
                return false;
        }
    }
    return true;
}

void initRootDir() {
    strncpy(rootDir.DIR_Name, "root       ", 11);
    rootDir.DIR_Attr = DIR;
    strncpy(rootDir.reserved, "          ", 10);
    rootDir.DIR_WrtTime = 0;
    rootDir.DIR_WrtDate = 0;
    rootDir.DIR_FstClus = 0;
    rootDir.DIR_FileSize = 0;
}

/*
 * 200 201 202 203 204 205 206 207
 * | 0  | 1  | | 2  |  3  | 4  |
 * 0: 200 = 0x200 + 0 * 3 / 2
 * 1: 201 = 0x200 + 1 * 3 / 2
 * 2: 203 = 0x200 + 2 * 3 / 2
 * 3: 204 = 0x200 + 3 * 3 / 2
 * */
u16 getNextClus(int clus) {
    u16 nxtClus;
    int i = FAT1Start + clus * 3 / 2;
    if (clus % 2 == 0) {
        nxtClus = IMG[i] + ((IMG[i + 1] & 0x0f) << 8);
    } else {
        nxtClus = ((IMG[i] & 0xf0) >> 4) + (IMG[i + 1] << 4);
    }
    return nxtClus;
}

void buildFileTree(DirOrFileEntry &dir) {
    array<char, 32> bytes{};
    if (strncmp(dir.DIR_Name, "root       ", 11) == 0) {
        for (int i = RootDirectoryStart; i < DataStart; i += 32) {
            for (int j = i; j < i + 32; j++) {
                bytes[j - i] = IMG[j];
            }
            if (((u8) bytes[11] == DIR &&
                 ((u8) bytes[26] + ((u8) bytes[27] << 8) >= 2))
                ||
                (((u8) bytes[11] == FL || (u8) bytes[11] == HDFL) && ((u8) bytes[26] + ((u8) bytes[27] << 8) >= 0))) {
                DirOrFileEntry *pdir = new struct DirOrFileEntry;
                strncpy(pdir->DIR_Name, bytes.data(), 11);
                pdir->DIR_Attr = (u8) bytes[11];
                pdir->DIR_FstClus = (u8) bytes[26];
                pdir->DIR_FstClus += (u8) bytes[27] << 8;
                pdir->DIR_FileSize = (u8) bytes[28];
                pdir->DIR_FileSize += (u8) bytes[29] << 8;
                pdir->DIR_FileSize += (u8) bytes[30] << 16;
                pdir->DIR_FileSize += (u8) bytes[31] << 24;

                bool BAD = false;
                for (int i = 0; i < 11; i++) {
                    if (!((bytes[i] >= '0' && bytes[i] <= '9')
                          || (bytes[i] >= 'a' && bytes[i] <= 'z')
                          || (bytes[i] >= 'A' && bytes[i] <= 'Z')
                          || bytes[i] == ' ' || bytes[i] == '_')) {
                        BAD = true;
                        break;
                    }
                }
                if (!BAD)
                    dir.subDir.push_back(pdir);
            }
        }
    }

    stack<queue<DirOrFileEntry *>> stk;
    queue<DirOrFileEntry *> que;
    for (DirOrFileEntry *subDir: dir.subDir) {
        if (subDir->DIR_Attr == DIR)
            que.push(subDir);
    }
    stk.push(que);
    while (!stk.empty()) {
        queue<DirOrFileEntry *> curQue = stk.top();

        bool POP = true;

        while (!curQue.empty()) {
            DirOrFileEntry *curDir = curQue.front();
            queue<DirOrFileEntry *> nxtQue;
            u16 clus = curDir->DIR_FstClus;

            while (clus < 0xff7) {
                int start = DataStart + (clus - 2) * 0x200;
                int end = start + 0x200;
                while (start < end) {
                    for (int i = 0; i < 32; i++) {
                        bytes[i] = IMG[start + i];
                    }
                    if ((bytes[11] == DIR &&
                         (((u8) bytes[26] + ((u8) bytes[27] << 8) >= 2) ||
                          strncmp(bytes.data(), ".          ", 11) == 0 ||
                          strncmp(bytes.data(), "..         ", 11) == 0))
                        ||
                        (((u8) bytes[11] == FL || (u8) bytes[11] == HDFL) &&
                         ((u8) bytes[26] + ((u8) bytes[27] << 8) >= 0))
                            ) {
                        DirOrFileEntry *pdir = new struct DirOrFileEntry;
                        strncpy(pdir->DIR_Name, bytes.data(), 11);
                        pdir->DIR_Attr = (u8) bytes[11];
                        pdir->DIR_FstClus = (u8) bytes[26];
                        pdir->DIR_FstClus += (u8) bytes[27] << 8;
                        pdir->DIR_FileSize = (u8) bytes[28];
                        pdir->DIR_FileSize += (u8) bytes[29] << 8;
                        pdir->DIR_FileSize += (u8) bytes[30] << 16;
                        pdir->DIR_FileSize += (u8) bytes[31] << 24;

                        bool BAD = false;
                        for (int i = 0; i < 11; i++) {
                            if (!((bytes[i] >= '0' && bytes[i] <= '9')
                                  || (bytes[i] >= 'a' && bytes[i] <= 'z')
                                  || (bytes[i] >= 'A' && bytes[i] <= 'Z')
                                  || bytes[i] == ' ' || bytes[i] == '_') &&
                                strncmp(bytes.data(), ".          ", 11) != 0 &&
                                strncmp(bytes.data(), "..         ", 11) != 0) {
                                BAD = true;
                                break;
                            }
                        }
                        if (!BAD) {
                            curDir->subDir.push_back(pdir);
                            if (pdir->DIR_Attr == DIR
                                && !(strncmp(pdir->DIR_Name, ".          ", 11) == 0 ||
                                     strncmp(pdir->DIR_Name, "..         ", 11) == 0)) {
                                nxtQue.push(pdir);
                            }
                        }
                    }
                    start += 32;
                }
                clus = getNextClus(clus);
            }

            if (!nxtQue.empty()) {
                stk.push(nxtQue);
                POP = false;
                break;
            } else {
                curQue.pop();
            }
        }
        if (POP) {
            stk.pop();
            if (!stk.empty() && !stk.top().empty())
                stk.top().pop();
        }
    }

}

string getName(const DirOrFileEntry &dir) {
    string ret;
    for (int i = 0; i < 8; i++) {
        if (dir.DIR_Name[i] == ' ')
            break;
        ret += dir.DIR_Name[i];
    }
    if (dir.DIR_Attr != DIR) {
        ret += '.';
        for (int i = 8; i < 11; i++) {
            if (dir.DIR_Name[i] == ' ')
                break;
            ret += dir.DIR_Name[i];
        }
        if (ret.size() <= 9 && ret[ret.size() - 1] == '.') {
            ret = ret.substr(0, ret.size() - 1);
        }
    }
    return ret;
}

void ls(const vector<string> &paths = {}) {
    string prefix;
    DirOrFileEntry *curDir = &rootDir;
    for (int i = 0; i < paths.size(); i++) {
        for (DirOrFileEntry *subDir: curDir->subDir) {
            if (paths[i] == getName(*subDir)) {
                if (i == paths.size() - 1) {
                    prefix += "/";
                    curDir = subDir;
                    break;
                } else if (i != paths.size() - 1 && subDir->DIR_Attr == DIR) {
                    prefix += "/" + getName(*subDir);
                    curDir = subDir;
                    break;
                } else {
//                    fprintf(stderr, "path error\n");
                    PRINT_RED("PATH ERROR!\n");
                }
            }
        }
    }


    stack<queue<DirOrFileEntry *>> stk;
    queue<DirOrFileEntry *> que;
    que.push(curDir);
    stk.push(que);
    string path = prefix;
    stack<string> nameStk;
    while (!stk.empty()) {
        queue<DirOrFileEntry *> curQue = stk.top();
        bool POP = true;

        while (!curQue.empty()) {
            DirOrFileEntry *curDir = curQue.front();
            queue<DirOrFileEntry *> nxtQue;
            if (strncmp(curDir->DIR_Name, "root       ", 11) == 0) {
                nameStk.push("");
                path += "/";
//                cout << path << ":" << endl;
                PRINT_WHITE(path.c_str());
                PRINT_WHITE(":\n");
            } else {
                nameStk.push(getName(*curDir));
                path += getName(*curDir) + "/";
//                printf("%s:\n", path.c_str());
                PRINT_WHITE(path.c_str());
                PRINT_WHITE(":\n");
            }
            for (DirOrFileEntry *subDir: curDir->subDir) {
                if (subDir->DIR_Attr == DIR) {
//                    cout << RED << getName(*subDir) << "  " << RESET;
                    PRINT_RED(getName(*subDir).c_str());
                    PRINT_WHITE("  ");
                    if (subDir->DIR_Attr == DIR
                        && !(strncmp(subDir->DIR_Name, ".          ", 11) == 0 ||
                             strncmp(subDir->DIR_Name, "..         ", 11) == 0)) {
                        nxtQue.push(subDir);
                    }
                } else {
//                    cout << getName(*subDir) << "  ";
                    PRINT_WHITE(getName(*subDir).c_str());
                    PRINT_WHITE("  ");
                }
            }
//            cout << endl;
            PRINT_WHITE("\n");
            if (!nxtQue.empty()) {
                stk.push(nxtQue);
                POP = false;
                break;
            } else {
                curQue.pop();
                path = path.size() == 0 ? path : path.substr(0, (path.size() - nameStk.top().size() - 1));
                if (!nameStk.empty())
                    nameStk.pop();
            }
        }
        if (POP) {
            stk.pop();
            if (!stk.empty() && !stk.top().empty())
                stk.top().pop();
            //path = path.size() == 0 ? path : path.substr(0, (path.size() - nameStk.top().size() - 1));
            path = path.size() == 0 ? path : path.substr(0,
                                                         (path.size() - (nameStk.empty() ? 0 : nameStk.top().size()) -
                                                          1));
            if (!nameStk.empty())
                nameStk.pop();
        }
    }

}


pair<int, int> getDirAndFileCnt(const DirOrFileEntry &dir) {
    int cnt = 0;
    for (const DirOrFileEntry *subDir: dir.subDir) {
        if (subDir->DIR_Attr == 0x10) {
            cnt += 1;
        }
    }
    return {cnt, dir.subDir.size() - cnt};
}

void ls_l(const string &prefix, DirOrFileEntry dir) {
    stack<queue<DirOrFileEntry *>> stk;
    queue<DirOrFileEntry *> que;
    que.push(&dir);
    stk.push(que);
    string path = prefix;
    stack<string> nameStk;

    while (!stk.empty()) {
        queue<DirOrFileEntry *> curQue = stk.top();
        bool POP = true;

        while (!curQue.empty()) {
            DirOrFileEntry *curDir = curQue.front();
            queue<DirOrFileEntry *> nxtQue;
            if (strncmp(curDir->DIR_Name, "root       ", 11) == 0) {
                nameStk.push("");
                path += "/";
//                printf("%s %d %d:\n", path.c_str(), getDirAndFileCnt(*curDir).first, getDirAndFileCnt(*curDir).second);
                PRINT_WHITE(path.c_str());
                PRINT_WHITE(" ");
                PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).first).c_str());
                PRINT_WHITE(" ");
                PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).second).c_str());
                PRINT_WHITE(":\n");
            } else {
                nameStk.push(getName(*curDir));
                path += getName(*curDir) + "/";
//                printf("%s %d %d:\n", path.c_str(), getDirAndFileCnt(*curDir).first - 2, getDirAndFileCnt(*curDir).second);
                PRINT_WHITE(path.c_str());
                PRINT_WHITE(" ");
                PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).first - 2).c_str());
                PRINT_WHITE(" ");
                PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).second).c_str());
                PRINT_WHITE(":\n");
            }
            for (DirOrFileEntry *subDir: curDir->subDir) {
                if (subDir->DIR_Attr == DIR) {
                    if (!(strncmp(subDir->DIR_Name, ".          ", 11) == 0 ||
                          strncmp(subDir->DIR_Name, "..         ", 11) == 0)) {
//                        printf("%s%s%s  %d %d\n", RED, getName(*subDir).c_str(), RESET,
//                               getDirAndFileCnt(*subDir).first - 2,
//                               getDirAndFileCnt(*subDir).second);
                        PRINT_RED(getName(*subDir).c_str());
                        PRINT_WHITE(" ");
                        PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).first - 2).c_str());
                        PRINT_WHITE(" ");
                        PRINT_WHITE(to_string(getDirAndFileCnt(*curDir).second).c_str());
                        PRINT_WHITE("\n");

                    } else {
//                        printf("%s%s%s\n", RED, getName(*subDir).c_str(), RESET);
                        PRINT_RED(getName(*subDir).c_str());
                        PRINT_WHITE("\n");
                    }
                    if (subDir->DIR_Attr == DIR
                        && !(strncmp(subDir->DIR_Name, ".          ", 11) == 0 ||
                             strncmp(subDir->DIR_Name, "..         ", 11) == 0)) {
                        nxtQue.push(subDir);
                    }
                } else {
//                    printf("%s  %d\n", getName(*subDir).c_str(), subDir->DIR_FileSize);
                    PRINT_WHITE(getName(*subDir).c_str());
                    PRINT_WHITE(" ");
                    PRINT_WHITE(to_string(subDir->DIR_FileSize).c_str());
                    PRINT_WHITE("\n");
                }
            }
            PRINT_WHITE("\n");
            if (!nxtQue.empty()) {
                stk.push(nxtQue);
                POP = false;
                break;
            } else {
                curQue.pop();
                path = path.size() == 0 ? path : path.substr(0,
                                                             (path.size() -
                                                              (nameStk.empty() ? 0 : nameStk.top().size()) -
                                                              1));
                if (!nameStk.empty())
                    nameStk.pop();
            }
        }
        if (POP) {
            stk.pop();
            if (!stk.empty() && !stk.top().empty())
                stk.top().pop();
            path = path.size() == 0 ? path : path.substr(0,
                                                         (path.size() - (nameStk.empty() ? 0 : nameStk.top().size()) -
                                                          1));
            if (!nameStk.empty())
                nameStk.pop();
        }
    }
}


void ls_path_l(const vector<string> &paths) {
    string prefix;
    const DirOrFileEntry *curDir = &rootDir;
    for (int i = 0; i < paths.size(); i++) {
        for (const DirOrFileEntry *subDir: curDir->subDir) {
            if (paths[i] == getName(*subDir)) {
                if (i == paths.size() - 1) {
                    prefix += "/";
                    ls_l(prefix, *subDir);
                } else if (i != paths.size() - 1 && subDir->DIR_Attr == DIR) {
                    prefix += "/" + getName(*subDir);
                    curDir = subDir;
                    break;
                } else {
                    PRINT_RED("PATH ERROR\n");
                }
            }
        }
    }
}

void cat(u16 clus, int size) {
    if (clus == 0xff7) {
        PRINT_RED("CLUS ERROR\n");
    } else if (clus >= 0xff8) {
        return;
    }
    int start = DataStart + 0x200 * (clus - 2);
    if (size >= 512) {
        for (int i = start; i < start + 512; i++) {
//            printf("%c", (char) IMG[i]);
            string c;
            c += (char) IMG[i];
            PRINT_WHITE(c.c_str());
        }
        size -= 512;
        clus = getNextClus(clus);
        cat(clus, size);
    } else {
        for (int i = start; i < start + size; i++) {
//            printf("%c", IMG[i]);
            string c;
            c += (char) IMG[i];
            PRINT_WHITE(c.c_str());
        }
    }
}

void cat_path(const vector<string> &paths) {
    const DirOrFileEntry *curDir = &rootDir;
    for (int i = 0; i < paths.size(); i++) {
        for (const DirOrFileEntry *subDir: curDir->subDir) {
            if (paths[i] == getName(*subDir)) {
                if (i == paths.size() - 1 && (subDir->DIR_Attr == FL || subDir->DIR_Attr == HDFL)) {
                    cat(subDir->DIR_FstClus, subDir->DIR_FileSize);
                } else if (i != paths.size() - 1 && subDir->DIR_Attr == DIR) {
                    curDir = subDir;
                    break;
                } else {
                    PRINT_RED("PATH ERROR\n");
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    assert(argc == 2);
    if (!loadIMG(argv[1]))
        PRINT_RED("LOAD IMG ERROR\n");
    initRootDir();
    buildFileTree(rootDir);


    string args;
    while (true) {
        PRINT_WHITE("> ");
        getline(cin, args);
        vector<string> tokens = split(args, " ");
        if (tokens.empty()) {
            PRINT_RED("COMMAND ERROR\n");
            continue;
        }

        if (tokens[0] == "ls") {
            if (tokens.size() == 1) {
                ls();
            } else if (tokens.size() == 2) {
                bool flag = false;
                if (strncmp(tokens[1].c_str(), "-l", 2) == 0) {
                    for (int i = 1; i < tokens[1].size(); i++) {
                        if (tokens[1][i] != 'l') {
                            PRINT_RED("COMMAND ERROR\n");
                            flag = true;
                            break;
                        }
                    }
                    if (flag) {
                        continue;
                    }
                    ls_l("", rootDir);
                } else {
                    vector<string> paths = split(tokens[1], "/");
                    ls(paths);
                }
            } else if (tokens.size() == 3) {
                bool flag = false;
                if (strncmp(tokens[1].c_str(), "-l", 2) == 0) {
                    for (int i = 1; i < tokens[1].size(); i++) {
                        if (tokens[1][i] != 'l') {
                            PRINT_RED("COMMAND ERROR\n");
                            flag = true;
                            break;
                        }
                    }
                    if (flag) {
                        continue;
                    }
                    vector<string> paths = split(tokens[2], "/");
                    ls_path_l(paths);
                } else if (strncmp(tokens[2].c_str(), "-l", 2) == 0) {
                    for (int i = 1; i < tokens[2].size(); i++) {
                        if (tokens[2][i] != 'l') {
                            PRINT_RED("COMMAND ERROR\n");
                            break;
                        }
                    }
                    vector<string> paths = split(tokens[1], "/");
                    ls_path_l(paths);
                } else {
                    PRINT_RED("COMMAND ERROR\n");
                }
            } else if (tokens.size() == 4) {
                bool flag = false;
                for (int i = 1; i < tokens[1].size(); i++) {
                    if (tokens[1][i] != 'l') {
                        flag = true;
                        PRINT_RED("COMMAND ERROR\n");
                        break;
                    }
                }
                if (flag) {
                    continue;
                }
                for (int i = 1; i < tokens[3].size(); i++) {
                    if (tokens[3][i] != 'l') {
                        flag = true;
                        PRINT_RED("COMMAND ERROR\n");
                        break;
                    }
                }
                if (flag) {
                    continue;
                }
                vector<string> paths = split(tokens[2], "/");
                ls_path_l(paths);
            } else {
                PRINT_RED("COMMAND ERROR\n");
            }
        } else if (tokens[0] == "cat" && tokens.size() == 2) {
            vector<string> paths = split(tokens[1], "/");
            cat_path(paths);
        } else if (tokens[0] == "exit" && tokens.size() == 1) {
            exit(0);
        } else {
            PRINT_RED("COMMAND ERROR\n");
        }
    }
    return 0;
}

