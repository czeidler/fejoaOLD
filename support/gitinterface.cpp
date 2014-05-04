#include "gitinterface.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSharedPointer>
#include <QTextStream>

class PackManager {
public:
    PackManager(GitInterface *gitInterface, git_repository *repository, git_odb *objectDatabase);

    WP::err exportPack(QByteArray &pack, const QString &commitOldest, const QString &endCommit, const QString &ignoreCommit, int format = -1) const;
    WP::err importPack(const QByteArray &data, const QString &base, const QString &last);

private:
    int readTill(const QByteArray &in, QString &out, int start, char stopChar);

    void findMissingObjects(QList<QString> &listOld, QList<QString> &listNew, QList<QString> &missing) const;
    //! Collect all ancestors including the start $commit.
    WP::err collectAncestorCommits(const QString &commit, QList<QString> &ancestors) const;
    WP::err collectMissingBlobs(const QString &commitStop, const QString &commitLast, const QString &ignoreCommit, QList<QString> &blobs, int type = -1) const;
    WP::err packObjects(const QList<QString> &objects, QByteArray &out) const;
    WP::err listTreeObjects(const git_oid *treeId, QList<QString> &objects) const;

    WP::err mergeBranches(const QString &baseCommit, const QString &ours, const QString &theirs, QString &merge);

    WP::err mergeCommit(const git_oid *treeOid, git_commit *parent1, git_commit *parent2);
private:
    GitInterface *database;
    git_repository *repository;
    git_odb *objectDatabase;
};


QString oidToQString(const git_oid *oid)
{
    char buffer[GIT_OID_HEXSZ+1];
    git_oid_tostr(buffer, GIT_OID_HEXSZ+1, oid);
    return QString(buffer);
}

void oidFromQString(git_oid *oid, const QString &string)
{
    QByteArray data = string.toLatin1();
    git_oid_fromstrn(oid, data.data(), data.count());
}

PackManager::PackManager(GitInterface *gitInterface, git_repository *repository, git_odb *objectDatabase) :
    database(gitInterface),
    repository(repository),
    objectDatabase(objectDatabase)
{
}

WP::err PackManager::importPack(const QByteArray& data, const QString &base, const QString &last)
{
    int objectStart = 0;
    while (objectStart < data.length()) {
        QString hash;
        QString size;
        int objectEnd = objectStart;
        objectEnd = readTill(data, hash, objectEnd, ' ');
        objectEnd = readTill(data, size, objectEnd, '\0');
        int blobStart = objectEnd;
        objectEnd += size.toInt();

        const char* dataPointer = data.data() + blobStart;
        WP::err error = database->writeFile(hash, dataPointer, objectEnd - blobStart);
        if (error != WP::kOk)
            return error;

        objectStart = objectEnd;
    }

    QString currentTip = database->getTip();
    QString newTip = last;
    if (currentTip != base) {
        WP::err error = mergeBranches(base, currentTip, last, newTip);
        if (error != WP::kOk)
            return error;
    }
    // update tip
    return database->updateTip(newTip);
}

int PackManager::readTill(const QByteArray& in, QString &out, int start, char stopChar)
{
    int pos = start;
    while (pos < in.length() && in.at(pos) != stopChar) {
        out.append(in.at(pos));
        pos++;
    }
    pos++;
    return pos;
}

void PackManager::findMissingObjects(QList<QString> &listOld, QList<QString>& listNew, QList<QString> &missing) const
{
    qSort(listOld.begin(), listOld.end());
    qSort(listNew.begin(), listNew.end());

    int a = 0;
    int b = 0;
    while (a < listOld.count() || b < listNew.count()) {
        int cmp;
        if (a < listOld.count() && b < listNew.count())
            cmp = listOld[a].compare(listNew[b]);
        else
            cmp = 0;
        if (b >= listNew.count() || cmp < 0)
            a++;
        else if (a >= listOld.count() || cmp > 0) {
            missing.append(listNew[b]);
            b++;
        } else {
            a++;
            b++;
        }
    }
}

WP::err PackManager::collectAncestorCommits(const QString &commit, QList<QString> &ancestors) const {
    QList<QString> commits;
    commits.append(commit);
    while (commits.count() > 0) {
        QString currentCommit = commits.takeFirst();
        if (ancestors.contains(currentCommit))
            continue;
        ancestors.append(currentCommit);

        // collect parents
        git_commit *commitObject;
        git_oid currentCommitOid;
        oidFromQString(&currentCommitOid, currentCommit);
        if (git_commit_lookup(&commitObject, repository, &currentCommitOid) != 0)
            return WP::kError;
        for (unsigned int i = 0; i < git_commit_parentcount(commitObject); i++) {
            const git_oid *parent = git_commit_parent_id(commitObject, i);

            QString parentString = oidToQString(parent);
            commits.append(parentString);
        }
        git_commit_free(commitObject);
    }
    return WP::kOk;
}

WP::err PackManager::collectMissingBlobs(const QString &commitStop, const QString &commitLast, const QString &ignoreCommit, QList<QString> &blobs, int type) const {
    QList<QString> commits;
    QList<QString> newObjects;
    commits.append(commitLast);
    QList<QString> stopAncestorCommits;
    if (ignoreCommit != "")
        stopAncestorCommits.append(ignoreCommit);
    bool stopAncestorsCalculated = false;
    while (commits.count() > 0) {
        QString currentCommit = commits.takeFirst();
        if (currentCommit == commitStop)
            continue;
        if (newObjects.contains(currentCommit))
            continue;
        newObjects.append(currentCommit);

        // collect tree objects
        git_commit *commitObject;
        git_oid currentCommitOid;
        oidFromQString(&currentCommitOid, currentCommit);
        if (git_commit_lookup(&commitObject, repository, &currentCommitOid) != 0)
            return WP::kError;
        WP::err status = listTreeObjects(git_commit_tree_id(commitObject), newObjects);
        if (status != WP::kOk) {
            git_commit_free(commitObject);
            return WP::kError;
        }

        // collect parents
        unsigned int parentCount = git_commit_parentcount(commitObject);
        if (parentCount > 1 && !stopAncestorsCalculated) {
            collectAncestorCommits(commitStop, stopAncestorCommits);
            stopAncestorsCalculated = true;
        }
        for (unsigned int i = 0; i < parentCount; i++) {
            const git_oid *parent = git_commit_parent_id(commitObject, i);

            QString parentString = oidToQString(parent);
            // if we reachted the ancestor commits tree we are done
            if (!stopAncestorCommits.contains(parentString))
                commits.append(parentString);
        }
        git_commit_free(commitObject);
    }

    // get stop commit object tree
    QList<QString> stopCommitObjects;
    if (commitStop != "") {
        git_commit *stopCommitObject;
        git_oid stopCommitOid;
        oidFromQString(&stopCommitOid, commitStop);
        if (git_commit_lookup(&stopCommitObject, repository, &stopCommitOid) != 0)
            return WP::kError;
        WP::err status = listTreeObjects(git_commit_tree_id(stopCommitObject), stopCommitObjects);
        git_commit_free(stopCommitObject);
        if (status != WP::kOk)
            return WP::kError;
    }

    // calculate the missing objects
    findMissingObjects(stopCommitObjects, newObjects, blobs);
    return WP::kOk;
}

#include "zlib.h"
#include <QBuffer>
#define CHUNK 16384

int gzcompress(QByteArray &source, QByteArray &outArray)
{
    QBuffer sourceBuffer(&source);
    sourceBuffer.open(QIODevice::ReadOnly);
    QBuffer outBuffer(&outArray);
    outBuffer.open(QIODevice::WriteOnly);

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    int status = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (status != Z_OK)
        return status;

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    int flush = Z_NO_FLUSH ;
    while (flush != Z_FINISH) {
        stream.avail_in = sourceBuffer.read((char*)in, CHUNK);
        flush = sourceBuffer.atEnd() ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = in;

        unsigned have;
        // deflate() till output buffer is full or we are done
        do {
            stream.avail_out = CHUNK;
            stream.next_out = out;
            status = deflate(&stream, flush); // no bad return value
            have = CHUNK - stream.avail_out;
            if (outBuffer.write((char*)out, have) != have) {
                (void)deflateEnd(&stream);
                return Z_ERRNO;
            }
        } while (stream.avail_out == 0);
    }

    deflateEnd(&stream);
    return Z_OK;
}

WP::err PackManager::packObjects(const QList<QString> &objects, QByteArray &out) const
{
    for (int i = 0; i < objects.count(); i++) {
        git_odb_object *object;
        git_oid objectOid;
        oidFromQString(&objectOid, objects.at(i));
        if (git_odb_read(&object, objectDatabase, &objectOid) != 0)
            return WP::kError;

        QByteArray blob;
        blob.append(git_object_type2string(git_odb_object_type(object)));
        blob.append(' ');
        QString sizeString;
        QTextStream(&sizeString) << git_odb_object_size(object);
        blob.append(sizeString);
        blob.append('\0');
        blob.append((char*)git_odb_object_data(object), git_odb_object_size(object));
        //blob = qCompress(blob);

        QByteArray blobCompressed;
        int error = gzcompress(blob, blobCompressed);
        if (error != Z_OK)
            return WP::kError;

        out.append(objects[i]);
        out.append(' ');
        QString blobSize;
        QTextStream(&blobSize) << blobCompressed.count();
        out.append(blobSize);
        out.append('\0');
        out.append(blobCompressed);
    }

    return WP::kOk;
}

WP::err PackManager::exportPack(QByteArray &pack, const QString &commitOldest, const QString &commitLatest, const QString &ignoreCommit, int format) const
{
    QString commitEnd(commitLatest);
    if (commitLatest == "")
        commitEnd = database->getTip();
    if (commitEnd == "")
        return WP::kNotInit;

    QList<QString> blobs;
    collectMissingBlobs(commitOldest, commitEnd, ignoreCommit, blobs);
    return packObjects(blobs, pack);
}

WP::err PackManager::listTreeObjects(const git_oid *treeId, QList<QString> &objects) const
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, repository, treeId);
    if (error != 0)
        return WP::kError;
    QString treeOidString = oidToQString(treeId);
    if (!objects.contains(treeOidString))
        objects.append(treeOidString);
    QList<git_tree*> treesQueue;
    treesQueue.append(tree);

    while (error == 0) {
        if (treesQueue.count() < 1)
            break;
        git_tree *currentTree = treesQueue.takeFirst();

        for (unsigned int i = 0; i < git_tree_entrycount(currentTree); i++) {
            const git_tree_entry *entry = git_tree_entry_byindex(currentTree, i);
            QString objectOidString = oidToQString(git_tree_entry_id(entry));
            if (!objects.contains(objectOidString))
                objects.append(objectOidString);
            if (git_tree_entry_type(entry) == GIT_OBJ_TREE) {
                git_tree *subTree;
                error = git_tree_lookup(&subTree, repository, git_tree_entry_id(entry));
                if (error != 0)
                    break;
                treesQueue.append(subTree);
            }
        }
        git_tree_free(currentTree);
    }
    if (error != 0) {
        for (int i = 0; i < treesQueue.count(); i++)
            git_tree_free(treesQueue[i]);
        return WP::kError;
    }
    return WP::kOk;
}

// copied from libgit2:
int git_merge_commits(
    git_index **out,
    git_repository *repo,
    const git_commit *our_commit,
    const git_commit *their_commit,
    const git_merge_tree_opts *opts)
{
    git_oid ancestor_oid;
    git_commit *ancestor_commit = NULL;
    git_tree *our_tree = NULL, *their_tree = NULL, *ancestor_tree = NULL;
    int error = 0;

    if ((error = git_merge_base(&ancestor_oid, repo, git_commit_id(our_commit), git_commit_id(their_commit))) < 0 &&
        error == GIT_ENOTFOUND)
        giterr_clear();
    else if (error < 0 ||
        (error = git_commit_lookup(&ancestor_commit, repo, &ancestor_oid)) < 0 ||
        (error = git_commit_tree(&ancestor_tree, ancestor_commit)) < 0)
        goto done;

    if ((error = git_commit_tree(&our_tree, our_commit)) < 0 ||
        (error = git_commit_tree(&their_tree, their_commit)) < 0 ||
        (error = git_merge_trees(out, repo, ancestor_tree, our_tree, their_tree, opts)) < 0)
        goto done;

done:
    git_commit_free(ancestor_commit);
    git_tree_free(our_tree);
    git_tree_free(their_tree);
    git_tree_free(ancestor_tree);

    return error;
}

static git_index_entry *index_entry_dup(const git_index_entry *source_entry)
{
    git_index_entry *entry;

    entry = (git_index_entry*)malloc(sizeof(git_index_entry));
    if (!entry)
        return NULL;

    memcpy(entry, source_entry, sizeof(git_index_entry));

    /* duplicate the path string so we own it */
    entry->path = strdup(source_entry->path);
    if (!entry->path)
        return NULL;

    return entry;
}

WP::err PackManager::mergeBranches(const QString &baseCommit, const QString &ours, const QString &theirs, QString &merge)
{
    git_oid oursOid;
    git_oid theirsOid;
    int error = git_oid_fromstr(&oursOid, ours.toLatin1());
    if (error != 0)
        return WP::kBadValue;
    error = git_oid_fromstr(&theirsOid, theirs.toLatin1());
    if (error != 0)
        return WP::kBadValue;

    git_commit *oursCommit;
    error = git_commit_lookup(&oursCommit, repository, &oursOid);
    if (error != 0)
        return WP::kEntryNotFound;
    git_commit *theirsCommit;
    error = git_commit_lookup(&theirsCommit, repository, &theirsOid);
    if (error != 0) {
        git_commit_free(oursCommit);
        return WP::kEntryNotFound;
    }

    // TODO: check if we could optimize since we know the base commit, i.e., use git_merge_trees
    git_index *mergeIndex;
    error = git_merge_commits(&mergeIndex, repository, oursCommit, theirsCommit, NULL);
    if (error != 0)
        return WP::kError;

    // fix conflicts
    git_index_conflict_iterator *iterator;
    error = git_index_conflict_iterator_new(&iterator, mergeIndex);
    if (error != 0) {
        git_commit_free(oursCommit);
        git_commit_free(theirsCommit);
        git_index_free(mergeIndex);
        return WP::kError;
    }
    QList<git_index_entry*> resolvedEntries;
    while (true) {
        const git_index_entry *ancestorOut;
        const git_index_entry *ourOut;
        const git_index_entry *theirOut;
        error = git_index_conflict_next(&ancestorOut, &ourOut, &theirOut, iterator);
        if (error == GIT_ITEROVER)
            break;
        if (error != 0) {
            git_commit_free(oursCommit);
            git_commit_free(theirsCommit);
            git_index_conflict_iterator_free(iterator);
            git_index_free(mergeIndex);
            return WP::kError;
        }
        // solve conflict
        const git_index_entry *selected = (ourOut != NULL) ? ourOut : theirOut;
        if (ourOut != NULL && theirOut != NULL
                && ourOut->mtime.nanoseconds > theirOut->mtime.nanoseconds)
            selected = theirOut;

        // duplicate the entry and remove the stage flag
        git_index_entry *editEntry = index_entry_dup(selected);
        editEntry->flags = (editEntry->flags & ~GIT_IDXENTRY_STAGEMASK) |
            ((0) << GIT_IDXENTRY_STAGESHIFT);

        resolvedEntries.append(editEntry);
    }
    git_index_conflict_iterator_free(iterator);

    git_index_conflict_cleanup(mergeIndex);

    // resolve index
    foreach (git_index_entry *entry, resolvedEntries) {
        error = git_index_add(mergeIndex, entry);
        if (error != 0) {
            git_commit_free(oursCommit);
            git_commit_free(theirsCommit);
            git_index_free(mergeIndex);
            return WP::kError;
        }

        free(entry->path);
        free(entry);
    }

    // write new root tree
    git_oid newRootTree;
    error = git_index_write_tree_to(&newRootTree, mergeIndex, repository);
    git_index_free(mergeIndex);
    if (error != 0) {
        git_commit_free(oursCommit);
        git_commit_free(theirsCommit);
        return WP::kError;
    }
    // commit
    WP::err wpError = mergeCommit(&newRootTree, oursCommit, theirsCommit);
    git_commit_free(oursCommit);
    git_commit_free(theirsCommit);
    if (wpError != WP::kOk)
        return wpError;
    merge = database->getTip();
    return WP::kOk;
}

WP::err PackManager::mergeCommit(const git_oid *treeOid, git_commit *parent1, git_commit *parent2)
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, repository, treeOid);
    if (error != 0)
        return WP::kError;

    QString refName = "refs/heads/";
    refName += database->branch();

    git_signature* signature;
    git_signature_new(&signature, "PackManager", "no mail", time(NULL), 0);

    git_commit *parents[2];
    parents[0] = parent1;
    parents[1] = parent2;
    const int nParents = 2;

    git_oid id;
    error = git_commit_create(&id, repository, refName.toLatin1().data(), signature, signature,
                              NULL, "merge", tree, nParents, (const git_commit**)parents);

    git_signature_free(signature);
    git_tree_free(tree);
    if (error != 0)
        return (WP::err)error;

    return WP::kOk;
}


bool GitInterface::sGitThreadsHaveBeeInit = false;

GitInterface::GitInterface()
    :
    repository(NULL),
    objectDatabase(NULL),
    currentBranch("master")
{
    newRootTreeOid.id[0] = '\0';
    if (!sGitThreadsHaveBeeInit) {
        git_threads_init();
        sGitThreadsHaveBeeInit = true;
    }
}


GitInterface::~GitInterface()
{
    unSet();
}

WP::err GitInterface::setTo(const QString &path, bool create)
{
    unSet();
    repositoryPath = path;

    int error = git_repository_open(&repository, repositoryPath.toLatin1().data());

    if (error != 0 && create)
        error = git_repository_init(&repository, repositoryPath.toLatin1().data(), true);

    if (error != 0)
        return (WP::err)error;

    return (WP::err)git_repository_odb(&objectDatabase, repository);
}

void GitInterface::unSet()
{
    git_repository_free(repository);
    git_odb_free(objectDatabase);
}

QString GitInterface::path()
{
    return repositoryPath;
}

WP::err GitInterface::setBranch(const QString &branch, bool /*createBranch*/)
{
    if (repository == NULL)
        return WP::kNotInit;
    currentBranch = branch;
    return WP::kOk;
}

QString GitInterface::branch() const
{
    return currentBranch;
}

WP::err GitInterface::write(const QString& path, const QByteArray &data)
{
    QString treePath = path;
    QString filename = removeFilename(treePath);

    git_oid oid;
    int error = git_odb_write(&oid, objectDatabase, data.data(), data.count(), GIT_OBJ_BLOB);
    if (error != WP::kOk)
        return (WP::err)error;
    git_filemode_t fileMode = GIT_FILEMODE_BLOB;

    git_tree *rootTree = NULL;
    if (newRootTreeOid.id[0] != '\0') {
        error = git_tree_lookup(&rootTree, repository, &newRootTreeOid);
        if (error != 0)
            return WP::kError;
    } else
        rootTree = getTipTree();

    while (true) {
        git_tree *node = NULL;
        if (rootTree == NULL || treePath == "")
            node = rootTree;
        else {
            git_tree_entry *treeEntry;
            error = git_tree_entry_bypath(&treeEntry, rootTree, treePath.toLatin1().data());
            if (error == 0) {
                error = git_tree_lookup(&node, repository, git_tree_entry_id(treeEntry));
                git_tree_entry_free(treeEntry);
                if (error != 0)
                    return WP::kError;
            }
        }

        git_treebuilder *builder = NULL;
        error = (WP::err)git_treebuilder_create(&builder, node);
        if (node != rootTree)
            git_tree_free(node);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            return (WP::err)error;
        }
        error = git_treebuilder_insert(NULL, builder, filename.toLatin1().data(), &oid, fileMode);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            git_treebuilder_free(builder);
            return (WP::err)error;
        }
        error = git_treebuilder_write(&oid, repository, builder);
        git_treebuilder_free(builder);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            return (WP::err)error;
        }

        // in the folloing we write trees
        fileMode = GIT_FILEMODE_TREE;
        if (treePath == "")
            break;
        filename = removeFilename(treePath);
    }

    git_tree_free(rootTree);
    newRootTreeOid = oid;
    return WP::kOk;
}

WP::err GitInterface::remove(const QString &path)
{
    // TODO implement
    return WP::kError;
}

WP::err GitInterface::commit()
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, repository, &newRootTreeOid);
    if (error != 0)
        return (WP::err)error;

    QString oldCommit = getTip();

    QString refName = "refs/heads/";
    refName += currentBranch;

    git_signature* signature;
    git_signature_new(&signature, "client", "no mail", time(NULL), 0);

    git_commit *tipCommit = getTipCommit();
    git_commit *parents[1];
    int nParents = 0;
    if (tipCommit) {
        parents[0] = tipCommit;
        nParents++;
    }

    git_oid id;
    error = git_commit_create(&id, repository, refName.toLatin1().data(), signature, signature,
                              NULL, "message", tree, nParents, (const git_commit**)parents);

    git_commit_free(tipCommit);
    git_signature_free(signature);
    git_tree_free(tree);
    if (error != 0)
        return (WP::err)error;

    newRootTreeOid.id[0] = '\0';

    emit newCommits(oldCommit, getTip());
    return WP::kOk;
}

WP::err GitInterface::read(const QString &path, QByteArray &data) const
{
    QString pathCopy = path;
    while (!pathCopy.isEmpty() && pathCopy.at(0) == '/')
        pathCopy.remove(0, 1);

    git_tree *rootTree = NULL;
    if (newRootTreeOid.id[0] != '\0') {
        int error = git_tree_lookup(&rootTree, repository, &newRootTreeOid);
        if (error != 0)
            return WP::kError;
    } else
        rootTree = getTipTree();

    if (rootTree == NULL)
        return WP::kNotInit;

    git_tree_entry *treeEntry;
    int error = git_tree_entry_bypath(&treeEntry, rootTree, pathCopy.toLatin1().data());
    if (error != 0)
        return WP::kError;

    git_blob *blob;
    error = git_blob_lookup(&blob, repository, git_tree_entry_id(treeEntry));
    git_tree_entry_free(treeEntry);
    if (error != 0)
        return WP::kEntryNotFound;

    data.clear();
    data.append((const char*)git_blob_rawcontent(blob), git_blob_rawsize(blob));

    git_blob_free(blob);
    return WP::kOk;
}

WP::err GitInterface::writeObject(const char *data, int size)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(data, size);
    QByteArray hashBin =  hash.result();
    QString hashHex =  hashBin.toHex();

    QByteArray arrayData;
    arrayData = QByteArray::fromRawData(data, size);
    QByteArray compressedData = qCompress(arrayData);

    return writeFile(hashHex, compressedData.data(), compressedData.size());
}

WP::err GitInterface::writeFile(const QString &hash, const char *data, int size)
{
    std::string hashHex = hash.toStdString();
    QString path;
    path.sprintf("%s/objects", repositoryPath.toStdString().c_str());
    QDir dir(path);
    dir.mkdir(hashHex.substr(0, 2).c_str());
    path += "/";
    path += hashHex.substr(0, 2).c_str();
    path += "/";
    path += hashHex.substr(2).c_str();
    QFile file(path);
    if (file.exists())
        return WP::kOk;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (file.write(data, size) < 0)
        return WP::kError;
    return WP::kOk;
}

QString GitInterface::getTip() const
{
    QString foundOid = "";
    QString refName = "refs/heads/";
    refName += currentBranch;

    git_strarray ref_list;
    git_reference_list(&ref_list, repository);

    for (unsigned int i = 0; i < ref_list.count; ++i) {
        const char *name = ref_list.strings[i];
        if (refName != name)
            continue;

        git_oid out;
        int error = git_reference_name_to_id(&out, repository, name);
        if (error != 0)
            break;
        char buffer[41];
        git_oid_fmt(buffer, &out);
        buffer[40] = '\0';
        foundOid = buffer;
        break;
    }

    git_strarray_free(&ref_list);

    return foundOid;
}


WP::err GitInterface::updateTip(const QString &commit)
{
    QString refPath = "refs/heads/";
    refPath += currentBranch;
    git_oid id;
    git_oid_fromstr(&id, commit.toLatin1().data());
    git_reference *newRef;
    int status = git_reference_create(&newRef, repository, refPath.toStdString().c_str(), &id, true);
    if (status != 0)
        return WP::kError;
    return WP::kOk;
}

QStringList GitInterface::listFiles(const QString &path) const
{
   return listDirectoryContent(path, GIT_OBJ_BLOB);
}

QStringList GitInterface::listDirectories(const QString &path) const
{
    return listDirectoryContent(path, GIT_OBJ_TREE);
}

QString GitInterface::getLastSyncCommit(const QString remoteName, const QString &remoteBranch) const
{
    QString refPath = repositoryPath;
    refPath += "/refs/remotes/";
    refPath += remoteName;
    refPath += "/";
    refPath += remoteBranch;

    QFile file(refPath);
    if (!file.open(QIODevice::ReadOnly))
        return "";
    QString oid;
    QTextStream outstream(&file);
    outstream >> oid;

    return oid;
}

WP::err GitInterface::updateLastSyncCommit(const QString remoteName, const QString &remoteBranch, const QString &uid) const
{
    QString refPath = repositoryPath;
    refPath += "/refs/remotes/";
    refPath += remoteName;
    QDir directory;
    if (!directory.mkpath(refPath))
        return WP::kError;
    refPath += "/";
    refPath += remoteBranch;

    QFile file(refPath);
    if (!file.open(QIODevice::WriteOnly))
        return WP::kError;
    file.resize(0);
    QTextStream outstream(&file);
    outstream << uid << "\n";

    return WP::kOk;
}

WP::err GitInterface::exportPack(QByteArray &pack, const QString &startCommit, const QString &endCommit, const QString &ignoreCommit, int format) const
{
    PackManager packManager((GitInterface*)this, repository, objectDatabase);
    return packManager.exportPack(pack, startCommit, endCommit, ignoreCommit, format);
}

WP::err GitInterface::importPack(const QByteArray &pack, const QString &baseCommit, const QString &endCommit, int format)
{
    PackManager packManager(this, repository, objectDatabase);
    WP::err error = packManager.importPack(pack, baseCommit, endCommit);
    if (error == WP::kOk)
        emit newCommits(baseCommit, getTip());
    return error;
}

git_tree *GitInterface::getCommitTree(const QString &commitHash) const {
    git_oid out;
    int error = git_oid_fromstr(&out, commitHash.toLatin1());
    if (error != 0)
        return NULL;
    git_commit *commit;
    error = git_commit_lookup(&commit, repository, &out);
    if (error != 0)
        return NULL;

    QSharedPointer<git_commit> commitDeleter(commit, git_commit_free);

    git_tree *tree = NULL;
    error = git_commit_tree(&tree, commit);
    if (error != 0)
        tree = NULL;
    return tree;
}

int diffFileHandler(const git_diff_delta *delta, float progress, void *payload) {
    DatabaseDiff *databaseDiff = (DatabaseDiff*)payload;

    switch (delta->status) {
    case GIT_DELTA_ADDED:
        databaseDiff->added.addPath(delta->new_file.path);
        break;
    case GIT_DELTA_MODIFIED:
        databaseDiff->modified.addPath(delta->new_file.path);
        break;
    case GIT_DELTA_DELETED:
        databaseDiff->removed.addPath(delta->new_file.path);
        break;
    default:
        return -1;
    }
    return 0;
}

int listTree(const char *root, const git_tree_entry *entry, void *payload) {
    QStringList *list = (QStringList*)payload;
    if (git_tree_entry_type(entry) == GIT_OBJ_BLOB)
        list->append(QString(root) + "/" +  git_tree_entry_name(entry));
    return 0;
}

void diff(git_tree *base, git_tree *end, DatabaseDiff &databaseDiff) {
    QStringList baseList;
    git_tree_walk(base, GIT_TREEWALK_PRE, listTree, &baseList);
    QStringList endList;
    git_tree_walk(end, GIT_TREEWALK_PRE, listTree, &endList);

    int a = 0;
    int b = 0;
    while (a < baseList.size() || b < endList.size()) {
        int cmp = 0;
        if (a < baseList.size() && b < endList.size())
            cmp = baseList.at(a).compare(endList.at(b));
        else
            cmp = 0;
        if (b >= endList.size() || cmp < 0) {
            // removed
            databaseDiff.removed.addPath(endList.at(a));
            a++;
        } else if (a >= endList.size() || cmp > 0) {
            // added
            databaseDiff.added.addPath(endList.at(b));
            b++;
        } else {
            a++;
            b++;
        }
    }
}

WP::err GitInterface::getDiff(const QString &baseCommit, const QString &endCommit, DatabaseDiff &databaseDiff)
{
    QSharedPointer<git_tree> baseTree(getCommitTree(baseCommit), git_tree_free);
    QSharedPointer<git_tree> endTree(getCommitTree(endCommit), git_tree_free);
    if (baseTree == NULL || endTree == NULL)
        return WP::kError;

    // use home grown diff method TODO: why the libgit2 version is not working?
    diff(baseTree.data(), endTree.data(), databaseDiff);
/*
    git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
    diffopts.flags = GIT_DIFF_FORCE_BINARY | GIT_DIFF_INCLUDE_UNTRACKED
            | GIT_DIFF_INCLUDE_UNMODIFIED | GIT_DIFF_INCLUDE_IGNORED
            | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

    git_diff_list *diff;
    int error = git_diff_tree_to_tree(&diff, repository, baseTree.data(), endTree.data(), &diffopts);
    if (error != 0)
        return WP::kError;
    QSharedPointer<git_diff_list>(diff, git_diff_list_free);

    error = git_diff_foreach(diff, diffFileHandler, NULL, NULL, &databaseDiff);
    if (error != 0)
        return WP::kError;
        */
    return WP::kOk;
}

QStringList GitInterface::listDirectoryContent(const QString &path, int type) const
{
    QStringList list;
    QSharedPointer<git_tree> tree(getDirectoryTree(path));
    if (tree == NULL)
        return list;
    int count = git_tree_entrycount(tree.data());
    for (int i = 0; i < count; i++) {
        const git_tree_entry *entry = git_tree_entry_byindex(tree.data(), i);
        if (type != -1 && git_tree_entry_type(entry) != type)
            continue;
        list.append(git_tree_entry_name(entry));
    }
    return list;
}

git_commit *GitInterface::getTipCommit() const
{
    QString tip = getTip();
    git_oid out;
    int error = git_oid_fromstr(&out, tip.toLatin1());
    if (error != 0)
        return NULL;
    git_commit *commit;
    error = git_commit_lookup(&commit, repository, &out);
    if (error != 0)
        return NULL;
    return commit;
}

git_tree *GitInterface::getTipTree() const
{
    git_tree *rootTree = NULL;
    git_commit *commit = getTipCommit();
    if (commit != NULL) {
        int error = git_commit_tree(&rootTree, commit);
        git_commit_free(commit);
        if (error != 0) {
            printf("can't get tree from commit\n");
            return NULL;
        }
    }
    return rootTree;
}

git_tree *GitInterface::getDirectoryTree(const QString &dirPath) const
{
    git_tree *tree = getTipTree();
    if (tree == NULL)
        return tree;

    QString dir = dirPath.trimmed();
    while (!dir.isEmpty() && dir.at(0) == '/')
        dir.remove(0, 1);
    while (!dir.isEmpty() && dir.at(dir.count() - 1) == '/')
        dir.remove(dir.count() - 1, 1);
    if (dir.isEmpty())
        return tree;

    while (!dir.isEmpty()) {
        int slash = dir.indexOf("/");
        QString subDir;
        if (slash < 0) {
            subDir = dir;
            dir = "";
        } else {
            subDir = dir.left(slash);
            dir.remove(0, slash + 1);
        }
        const git_tree_entry *entry = git_tree_entry_byname(tree, subDir.toLatin1().data());
        if (entry == NULL || git_tree_entry_type(entry) != GIT_OBJ_TREE) {
            git_tree_free(tree);
            return NULL;
        }
        const git_oid *oid = git_tree_entry_id(entry);
        git_tree *newTree = NULL;
        int error = git_tree_lookup(&newTree, repository, oid);
        if (error != 0) {
            git_tree_free(tree);
            return NULL;
        }
        tree = newTree;
    }
    return tree;
}

QString GitInterface::removeFilename(QString &path) const
{
    path = path.trimmed();
    while (!path.isEmpty() && path.at(0) == '/')
        path.remove(0, 1);
    if (path.isEmpty())
        return "";
    while (true) {
        int index = path.lastIndexOf("/", -1);
        if (index < 0) {
            QString filename = path;
            path.clear();
            return filename;
        }

        // ignore multiple slashes
        if (index == path.count() - 1) {
            path.truncate(path.count() -1);
            continue;
        }
        int filenameLength = path.count() - 1 - index;
        QString filename = path.mid(index + 1, filenameLength);
        path.truncate(index);
        while (!path.isEmpty() && path.at(path.count() - 1) == '/')
            path.remove(path.count() - 1, 1);
        return filename;
    }
    return path;
}
