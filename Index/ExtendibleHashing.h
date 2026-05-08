//
// Created by Usuario on 4/5/2026.
//

#ifndef EXTENDIBLEHASHING_H
#define EXTENDIBLEHASHING_H
#include <vector>
#include <bits/stdc++.h>

using namespace std;

struct RID_h {int page_id ; int slot;};
static constexpr RID_h NULLRID  = {-1,-1};
inline bool isNullRID(const RID_h& r) {return r.page_id == -1 && r.slot == -1;}

static constexpr int PAGE_SIZE_H = 4096;
using PageID = int;
static constexpr PageID NULL_PAGE_H = -1;
struct Page_h { char data[PAGE_SIZE_H]; };

static constexpr int DLIMIT       = 16;
//--------------------------------FixedString---------------------------------------
template<int N>
struct FixedString_H {
    char data[N];

    FixedString_H() {
        memset(data, 0, N);
    }

    FixedString_H(const char* s) {
        memset(data, 0, N);
        memcpy(data, s, min((int)strlen(s), N));
    }

    FixedString_H(const std::string& s) {
        memset(data, 0, N);
        memcpy(data, s.c_str(), min((int)s.size(), N));
    }

    bool operator<(const FixedString_H& other) const {
        return strncmp(data, other.data, N) < 0;
    }

    bool operator>(const FixedString_H& other) const {
        return strncmp(data, other.data, N) > 0;
    }

    bool operator==(const FixedString_H& other) const {
        return strncmp(data, other.data, N) == 0;
    }

    bool operator<=(const FixedString_H& other) const {
        return !(*this > other);
    }

    bool operator>=(const FixedString_H& other) const {
        return !(*this < other);
    }
};
namespace std {
    template<int N>
    struct hash<FixedString_H<N>> {
        size_t operator()(const FixedString_H<N>& fs) const {
            size_t h = 0;
            for (int i = 0; i < N && fs.data[i] != '\0'; i++) {
                h = h * 31 + static_cast<unsigned char>(fs.data[i]);
            }
            return h;
        }
    };
}
//-----------------------------------DISK---------------------------------------------

//Página 0  →  meta (nextPage + dirPageId + D)
//Página 1  →  primer bucket
//Página 2  →  segundo bucket

class Diske {
private:
    fstream file;
    int nextPage = -1; // pagina 0
    static constexpr int META_OFFSET_NEXT = 0;
    static constexpr int META_OFFSET_DIRPAGE = sizeof(int);
    static constexpr int META_OFFSET_D = sizeof(int)  + sizeof(PageID);

    int readCount = 0;
    int writeCount = 0;

    void loadMeta() {
        file.clear();
        file.seekg(0,ios::end);
        streamsize sz = file.tellg();

        if (sz < PAGE_SIZE_H) {
            nextPage = 1;
            saveMeta(NULL_PAGE_H,1);
            return;
        }
        Page_h p{};
        file.clear();
        file.seekg(0);
        file.read(p.data, PAGE_SIZE_H);
        memcpy(&nextPage, p.data + META_OFFSET_NEXT,sizeof(int));
    }

public:
    explicit Diske(const string& filename) {
        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
        }
        loadMeta();
    }

    Page_h read(PageID id) {
        ++readCount;
        Page_h p{};
        file.clear();
        file.seekg((long long)id * PAGE_SIZE_H);
        file.read(p.data, PAGE_SIZE_H);
        if (!file) {
            cerr << "ERROR: read fallo en página " << id << "\n";
        }
        return p;
    }

    void write(PageID id, const Page_h& p) {
        ++writeCount;
        file.clear();
        file.seekp((long long)id * PAGE_SIZE_H);
        file.write(p.data, PAGE_SIZE_H);
        file.flush();
        if (!file) {
            cerr << "ERROR: write fallo en página " << id << "\n";
        }
    }

    PageID alloc() {
        PageID id = nextPage++;
        Page_h p{};
        write(id, p);       // inicializar la página en disco
        return id;
    }

    void saveMeta(PageID dirPageId, int D) {
        Page_h p{};

        file.clear();
        file.seekg(0, ios::end);
        streamsize sz = file.tellg();

        if (sz >= PAGE_SIZE_H) {
            file.clear();
            file.seekg(0);
            file.read(p.data, PAGE_SIZE_H);
        }
        memcpy(p.data + META_OFFSET_NEXT,    &nextPage,  sizeof(int));
        memcpy(p.data + META_OFFSET_DIRPAGE, &dirPageId, sizeof(PageID));
        memcpy(p.data + META_OFFSET_D,       &D,         sizeof(int));

        file.clear();
        file.seekp(0);
        file.write(p.data, PAGE_SIZE_H);
        file.flush();
    }

    pair<PageID, int> loadMetaValues() {
        Page_h p{};
        file.seekg(0);
        file.read(p.data, PAGE_SIZE_H);

        PageID dirPageId = NULL_PAGE_H;
        int    D         = 1;
        memcpy(&dirPageId, p.data + META_OFFSET_DIRPAGE, sizeof(PageID));
        memcpy(&D,         p.data + META_OFFSET_D,       sizeof(int));

        return { dirPageId, D };
    }

    void saveDirectory(PageID& dirPageId,int D,const vector<PageID>& directory) {
        if (dirPageId == NULL_PAGE_H)
            dirPageId = alloc();

        Page_h p{};
        int size = (int)directory.size();
        memcpy(p.data, &size, sizeof(int));
        for (int i = 0; i < size; i++)
            memcpy(p.data + sizeof(int) + i * sizeof(PageID),
                   &directory[i], sizeof(PageID));

        write(dirPageId, p);
        saveMeta(dirPageId, D);
    }

    vector<PageID> loadDirectory(PageID dirPageId) {
        vector<PageID> directory;
        if (dirPageId == NULL_PAGE_H) return directory;

        Page_h p   = read(dirPageId);
        int size = 0;
        memcpy(&size, p.data, sizeof(int));

        directory.resize(size);
        for (int i = 0; i < size; i++)
            memcpy(&directory[i],
                   p.data + sizeof(int) + i * sizeof(PageID),
                   sizeof(PageID));

        return directory;
    }

    void resetCounters()      { readCount = writeCount = 0; }
    int  totalReads()   const { return readCount;  }
    int  totalWrites()  const { return writeCount; }
    int  totalAccesses()const { return readCount + writeCount; }
    int  pageCount()    const { return nextPage; }
};
// -------------------------------------------------------------------------------------

template<typename TKey>
static constexpr int computeBucketCapacity() {
    // Overhead: count(int) + localDepth(int) + next(ptr) + useChaining(bool)
    constexpr int overhead = static_cast<int>(sizeof(int) + sizeof(int) + sizeof(PageID) + sizeof(bool));
    constexpr int leafEntry= static_cast<int>(sizeof(TKey) + sizeof(RID_h));
    return (PAGE_SIZE_H - overhead) / leafEntry;
}


template<typename TKey>
struct BucketPage {
    int count ;
    int  localDepth;
    PageID nextPage;
    bool useChaining;

    static constexpr int BUCKET_SIZE = computeBucketCapacity<TKey>();
    TKey keys[BUCKET_SIZE];
    RID_h  values[BUCKET_SIZE];

};

//--------------------------------------------------------------------------------------------------

template<typename TKey>
class ExtendibleHashing {
private:
    using BucketT = BucketPage<TKey>;

    Diske& disk;
    int D = 1; // global depth [1-16]
    vector<PageID> directory;  // en memoria mientras corre
    PageID dirPageId = NULL_PAGE_H;          // pagina donde se serializa el vector (disco)

    int getIndex(const TKey& key) {
        size_t h = hash<TKey>{}(key);
        return h & ((1 << D) - 1);     // se queda con los ultimos bits, para mapearlo en mi directorio
    }

    size_t getHash(const TKey& key) {
        return hash<TKey>{}(key);     // aplica el hash nada mas, sin hayar su lugar en directory
    }

public:

    explicit ExtendibleHashing(Diske& d): disk(d) {
        auto [dp, loadedD] = disk.loadMetaValues();
        if (dp == NULL_PAGE_H) {
            // primera vez — archivo nuevo
            D = 1;
            int size = 1 << D;
            directory.resize(size);

            // crear el único bucket inicial
            PageID firstBucket = disk.alloc();
            Page_h   p           = disk.read(firstBucket);
            BucketT* b         = asBucket(p);
            b->count           = 0;
            b->localDepth      = 0;
            b->nextPage        = NULL_PAGE_H;
            b->useChaining     = false;
            disk.write(firstBucket, p);

            // todas las entradas del directorio apuntan a ese bucket
            for (int i = 0; i < size; i++)
                directory[i] = firstBucket;
            saveDirectory();
        } else {
            // ya existía — reconstruir desde disco
            dirPageId = dp;
            D         = loadedD;
            directory = disk.loadDirectory(dirPageId);
        }
    }

    void expandDirectory() {
        int oldsize = 1 << D;
        D++;
        int newsize = 1 << D;

        directory.resize(newsize);

        for (int i=0; i < oldsize; i++) {
            directory[i + oldsize] = directory[i];
        }
        saveDirectory();
    }

    void split(int index) {
        PageID oldId = directory[index];
        Page_h p = disk.read(oldId);
        BucketT* oldBucket = asBucket(p);

        if (oldBucket->localDepth == D) {
            if (D>= DLIMIT) {
                oldBucket->useChaining = true;
                disk.write(oldId,p);
                return;
            }
            expandDirectory();
            p = disk.read(oldId);
            oldBucket = asBucket(p);
        }
        // Modificar local depth y definit splitbit
        int newLocalDepth = oldBucket->localDepth+1;
        int splitBit = 1 << (newLocalDepth - 1);

        // se guardar keys del old bucket
        vector<TKey> tempKeys;
        for (int i = 0 ; i < oldBucket->count; i++) {
            tempKeys.push_back(oldBucket->keys[i]);
        }
        oldBucket->count = 0;

        // borrando old bucket
        oldBucket->localDepth = newLocalDepth;
        oldBucket->count = 0;
        disk.write(oldId,p);

        // creando nuevo bucket
        PageID newId = disk.alloc();
        Page_h np = disk.read(newId);
        BucketT* newBucket = asBucket(np);
        newBucket->localDepth = newLocalDepth;
        newBucket->count = 0;
        newBucket->nextPage = NULL_PAGE_H;
        newBucket->useChaining = false;
        disk.write(newId,np);

        // reasignar entradas del directorio
        for (int i = 0; i < (int)directory.size(); i++) {
            if (directory[i] == oldId && (i & splitBit)) {
                directory[i] = newId;
            }
        }

        for (TKey k:tempKeys) {
            size_t h = getHash(k);
            PageID destId = (h & splitBit) ? newId : oldId;

            Page_h dp = disk.read(destId);
            BucketT* db = asBucket(dp);
            db->keys[db->count++] = k;
            disk.write(destId,dp);
        }
        // persistir
        saveDirectory();
    }

    bool delete_aux(const TKey& key,PageID cubetaId) {
        while (cubetaId != NULL_PAGE_H) {
            Page_h p = disk.read(cubetaId);
            BucketT* cubeta = asBucket(p);

            for (int i = 0; i < cubeta->count; i++) {
                if (cubeta->keys[i] == key) {
                    for (int j = i; j < cubeta->count - 1; j++) {
                        cubeta->keys[j] = cubeta->keys[j + 1];
                    }
                    cubeta->count  = cubeta->count - 1;
                    disk.write(cubetaId,p);
                    return true;
                }
            }
            cubetaId = cubeta->nextPage;
        }
        return false;
    }

    PageID findOrMakeBucket(const TKey& key) {
        while (true) {
            int    index    = getIndex(key);
            PageID targetId = directory[index];
            PageID prevId   = NULL_PAGE_H;

            while (targetId != NULL_PAGE_H) {
                Page_h     p       = disk.read(targetId);
                BucketT* current = asBucket(p);

                if (!isFull(current)) return targetId;

                prevId   = targetId;
                targetId = current->nextPage;
            }

            // todos llenos — leer el bucket raíz para ver si tiene chaining
            Page_h     p = disk.read(directory[index]);
            BucketT* b = asBucket(p);

            if (!b->useChaining) {
                split(index);
                continue;  // reintentar desde el inicio
            }

            Page_h     pp = disk.read(prevId);
            BucketT* pb = asBucket(pp);

            PageID newId    = disk.alloc();
            Page_h   np       = disk.read(newId);
            BucketT* nb     = asBucket(np);
            nb->count       = 0;
            nb->localDepth  = pb->localDepth;
            nb->nextPage    = NULL_PAGE_H;
            nb->useChaining = true;
            disk.write(newId, np);

            pb->nextPage = newId;
            disk.write(prevId, pp);
            return newId;
        }
    }

    void insertIntoBucket(PageID page_id,const TKey& key,const RID_h& rid) {
        Page_h p = disk.read(page_id);
        BucketT* b = asBucket(p);
        b->keys [b->count] = key;
        b->values[b->count] = rid;
        b->count++;
        disk.write(page_id,p);
    }

    // -------------- insert fix -----------------
    void insert_hash(const TKey& key, const RID_h& rid) {
        insertIntoBucket(findOrMakeBucket(key),key,rid);
    }

    // --------------- search-hash ----------------
    vector<RID_h> search_hash(const TKey& key) {
        vector<RID_h> result;
        int index = getIndex(key);
        PageID target = directory[index];

        while (target != NULL_PAGE_H) {
            Page_h p = disk.read(target);
            BucketT* current = asBucket(p);

            for (int i = 0 ; i < current->count ; i++) {
                if (key == current->keys[i]) {
                    result.push_back({current->values[i]});
                }
            }
            target = current->nextPage;
        }
        return result;
    }

    // ---------------- remove-hash -----------
    void delete_hash(const TKey& key, const RID_h& target) {
        int    index  = getIndex(key);
        PageID currId = directory[index];

        while (currId != NULL_PAGE_H) {
            Page_h     p      = disk.read(currId);
            BucketT* bucket = asBucket(p);

            for (int i = 0; i < bucket->count; i++) {
                if (bucket->keys[i]        == key            &&
                    bucket->values[i].page_id == target.page_id &&
                    bucket->values[i].slot    == target.slot)
                {
                    // shift left para tapar el hueco
                    for (int j = i; j+1 < bucket->count; j++) {
                        bucket->keys  [j] = bucket->keys  [j+1];
                        bucket->values[j] = bucket->values[j+1];
                    }
                    bucket->count--;
                    disk.write(currId, p);
                    return;
                }
            }
            currId = bucket->nextPage;
        }
        cout << "No se encontro la entrada a borrar\n";
    }


    // helpers:
    BucketT* asBucket(Page_h& p) {
        return reinterpret_cast<BucketT*>(p.data);
    }
    bool isFull(BucketT* b) const { return b->count >= BucketT::BUCKET_SIZE;}

    void saveDirectory() {
        disk.saveDirectory(dirPageId, D, directory);
    }

};

/*
   void try_merge(Bucket<TKey>* bucket,int index) {
       if (bucket->localDepth == 0 || bucket->useChaining) return;

       int buddyIndex = index ^(1 << (bucket->localDepth - 1));
       Bucket<TKey>* buddy = directory[buddyIndex];
       if (buddy != bucket && buddy->localDepth == bucket->localDepth) {
           if (bucket->count + buddy->count <= BUCKET_SIZE) {
               for (int i = 0; i < buddy->count; i++) {
                   bucket->addKey(buddy->keys[i]);
               }
               for (int i = 0; i < directory.size(); i++) {
                   if (directory[i] == buddy) {
                       directory[i] = bucket;
                   }
               }
               bucket->localDepth--;
               delete buddy;

               //shrinkDirectory();
           }
       }
   }
*/


#endif //EXTENDIBLEHASHING_H
