<?php

include 'glip/lib/glip.php'; 


function isSHA1Bin($string) {
	if (strlen($string) == 20)
		return true;
	return false;
}

function isSHA1Hex($string) {
	if (strlen($string) == 40)
		return true;
	return false;
}

class TreeBuilder {
	private $rootTree;
	private $repo;
	private $newTrees;

	public function __construct($rootTree) {
		$this->rootTree = $rootTree;
		$this->repo = $rootTree->repo;
		$this->newTrees = array();
	}

	public function write() {
		foreach ($this->newTrees as $tree) {
			$ok = $tree->write();
			if (!$ok)
				return false;
		}
		$newTrees = array();
		return true;
	}

	public function updateFile($path, $object)
    {
		$this->_updateNode($this->rootTree, $path, 100644, $object);
    }

	public function updateNode($path, $mode, $object)
    {
		$this->_updateNode($this->rootTree, $path, $mode, $object);
    }

	public function _updateNode($tree, $path, $mode, $object)
    {
        if (!is_array($path))
            $path = explode('/', $path);
        $name = array_shift($path);
        if (count($path) == 0)
        {
            /* create leaf node */
            if ($mode)
            {
                $node = new stdClass;
                $node->mode = $mode;
                $node->name = $name;
                $node->object = $object;
                $node->is_dir = !!($mode & 040000);

                $tree->nodes[$node->name] = $node;
            }
            else
                unset($tree->nodes[$name]);
        }
        else
        {
			$wasInList = false;
            /* descend one level */
            if (isset($tree->nodes[$name]))
            {
                $node = $tree->nodes[$name];
                if (!$node->is_dir)
                    throw new GitTreeInvalidPathError;
                $subtree = $this->_findSubTree($node->object);
                if ($subtree == NULL)
					$subtree = clone $this->repo->getObject($node->object);
				else
					$wasInList = true;
            }
            else
            {
                /* create new tree */
                $subtree = new GitTree($this->repo);

                $node = new stdClass;
                $node->mode = 040000;
                $node->name = $name;
                $node->is_dir = TRUE;

                $tree->nodes[$node->name] = $node;
            }
            $this->_updateNode($subtree, $path, $mode, $object);

            $subtree->rehash();
            $node->object = $subtree->getName();

            if (!$wasInList)
				$this->newTrees[] = $subtree;
        }
    }
   
    private function _findSubTree($name) {
		foreach ($this->newTrees as $tree) {
			if ($tree->getName() == $name)
				return $tree;
		}
		return NULL;
    }
}


class GitDatabase extends Git {
	public $dir;
	private $currentRootTree = null;

	public function __construct($repoPath) {
        $this->dir = $repoPath;
		$this->init();
        Git::__construct($repoPath);
    }

	private function init() {
		if (file_exists($this->dir))
			return false;
		mkdir($this->dir);
		mkdir($this->dir."/objects");
		mkdir($this->dir."/objects/pack");
		mkdir($this->dir."/refs");
		mkdir($this->dir."/refs/heads/");

		# set HEAD to master branch
		$f = fopen($this->dir."/HEAD", 'cb');
		flock($f, LOCK_EX);
		ftruncate($f, 0);
		$data = "ref: refs/heads/master";
		fwrite($f, $data);
		fclose($f);
		return true;
	}

	private function findLocalBranch($tiphex) {
		foreach (new DirectoryIterator($this->dir."/refs/heads/") as $file) {
			if($file->isDot())
				continue;
			$path = $this->dir."/refs/heads/".$file->getFilename();
			if (file_get_contents($path) == $tiphex)
				return $file->getFilename();
		}
		
		$path = sprintf('%s/packed-refs', $this->dir);
		if (file_exists($path))
		{
			$branch = NULL;
			$f = fopen($path, 'rb');
			flock($f, LOCK_SH);
			while ($branch === NULL && ($line = fgets($f)) != FALSE)
			{
				if ($line{0} == '#')
					continue;
				$parts = explode(' ', trim($line));
				if (count($parts) == 2 && $parts[0] == $tiphex) {
					if (preg_match('#^refs/heads/(.*)$#', $parts[1], $m))
						$branch = $m;
				}
			}
			fclose($f);
			if ($branch !== NULL)
				return $branch;

			throw new Exception(sprintf('no such branch with tip: %s', $tiphex));
		}
        return "";
	}

	/*! Returns true if $head is a branch name else $head is the hex id of the detached head. */
	public function getHead(&$head) {
		if (!file_exists($this->dir."/HEAD"))
			throw new Exception('HEAD not found');
		$f = fopen($this->dir."/HEAD", 'rb');
		flock($f, LOCK_SH);
		if (($line = fgets($f)) == FALSE)
			throw new Exception('No entry in HEAD');
		if (preg_match('#^ref: refs/heads/(.*)$#', $line, $head))
			return true;
		$head = $line;
		return false;
	}

	public function setBranchTip($branchName, $commitHex) {
		$f = fopen($this->dir."/refs/heads/$branchName", 'cb');
		flock($f, LOCK_SH);
		ftruncate($f, 0);
		fwrite($f, $commitHex."\n");
		fclose($f);
		return true;
	}

    //! \return hex tip commit
    public function getBranchTip($branchName) {
		if (!file_exists($this->dir."/refs/heads/$branchName"))
			return "";
		$f = fopen($this->dir."/refs/heads/$branchName", 'r');
		if (!$f)
			return "";
		flock($f, LOCK_SH);
		if (($line = fgets($f)) == FALSE)
			return "";
		if (!preg_match('#^ref: refs/heads/(.*)$#', $line, $tip))
			$tip = "";
		fclose($f);
		return $tip;
    }

	public function setHead($commitHex) {
		if (!file_exists($this->dir."/HEAD"))
			throw new Exception('HEAD not found');
		$f = fopen($this->dir."/HEAD", 'rb');
		flock($f, LOCK_SH);
		if (($line = fgets($f)) == FALSE)
			throw new Exception('No entry in HEAD');
		ftruncate($f, 0);
		fwrite($f, $commitHex);
		fclose($f);

		$branch = NULL;
		if ($line == "ref: refs/heads/master")
			$branch = master;
		else
			$branch = findLocalBranch($line);

		if ($branch !== NULL) {
			$f = fopen($this->dir."/refs/heads/$branch", 'rb');
			flock($f, LOCK_SH);
			ftruncate($f, 0);
			fwrite($f, $commitHex);
			fclose($f);
		}
	}

	// Returns the root tree object.
	public function getRootTree($branch) {
		$rootTree = NULL;
		try {
			$tip = $this->getTip($branch);
			$rootCommit = $this->getObject($tip);
			$rootTree = clone $this->getObject($rootCommit->tree);
		} catch (Exception $e) {
			$rootTree = new GitTree($this);
		}
		return $rootTree;
	}

	public function writeBlob($data) {
		$object = new GitBlob($this);
		$object->data = $data;
		$object->rehash();
		$object->write();
		return $object;
	}

	public function printObject($name) {
		$object = $this->getObject($name);
		if ($object->getType() == Git::OBJ_TREE) {
			echo "Object Tree: ".sha1_hex($object->getName())."<br>";
			foreach ($object->nodes as $node)
				echo "-node: ".$node->name."<br>";
		} else if ($object->getType() == Git::OBJ_COMMIT) {
			echo "Object Commit: ".sha1_hex($object->getName())."<br>";
		} else if ($object->getType() == Git::OBJ_BLOB) {
			echo "Object Blob: ".sha1_hex($object->getName())."<br>";
		}
	}

	public function readBlobContent($branch, $path) {
		$tree = $this->getRootTree($branch);
		$blobId = $tree->find($path);
		if ($blobId === null)
			return null;
		$blob = $this->getObject($blobId);
		if ($blob === null)
			return null;
		return $blob->data;
	}

	// Stores the writes into $this->currentRootTree. This tree can be committed using commit()
	public function write($branch, $path, $data) {
		if ($this->currentRootTree === null)
			$this->currentRootTree = $this->getRootTree($branch);
		$treeBuilder = new TreeBuilder($this->currentRootTree);
		# build new tree
		$object = $this->writeBlob($data);
		$treeBuilder->updateFile($path, $object->getName());
		$ok = $treeBuilder->write();
		if (!$ok)
			return false;
		$this->currentRootTree->rehash();
		return $this->currentRootTree->write();
	}

	public function commit($branch) {
		if ($this->currentRootTree === null)
			return null;

		# write commit
		$parents = array();
		try {
			$tip = $this->getTip($branch);
			if (strlen($tip) > 0)
				$parents[] = $tip;
		} catch (Exception $e) {
		} 

		$commit = new GitCommit($this);
		$commit->tree = $this->currentRootTree->getName();
		$commit->parents = $parents;

		$commitStamp = new GitCommitStamp;
		$commitStamp->name = "server";
		$commitStamp->email = "no mail";
		$commitStamp->time = time();
		$commitStamp->offset = 0;

		$commit->author = $commitStamp;
		$commit->committer = $commitStamp;
		$commit->summary = "server commit";
		$commit->detail = "";

		$commit->rehash();
		$commit->write();

		$this->setBranchTip($branch, sha1_hex($commit->getName()));

		$this->currentRootTree = null;
		return $commit;
	}

	public function listDirectories($branch, $path) {
		$rootTree = $this->getRootTree($branch);
		$treeId = $rootTree->find($path);
		if ($treeId === null)
			return array();
		$tree = $this->getObject($treeId);

		$list = array();
		foreach ($tree->nodes as $node)
        {
            if (!$node->is_dir)
				continue;
			$list[] = $node->name;
		}

		return $list;
	}
}


class PackManager {
	public $repository;

	public function __construct($repository) {
		$this->repository = $repository;
	}

    /*
     * @param $branch (string) branch name
     * @param $commitOldest (string) start commit binary sha1
     * @param $commitLatest (string) end commit binary sha1
     */
	public function exportPack($branch, $commitOldest, &$commitLatest, $type) {
		if ($commitLatest == NULL)
			$commitLatest = $this->repository->getTip($branch);

		$blobs = $this->collectMissingBlobs($commitOldest, $commitLatest, $type);
		return $this->packObjects($blobs);
	}

	public function importPack($branch, $pack, $startCommit, $endCommit, $format = -1) {
		if (!isSHA1Hex($endCommit))
			return false;
		$objectStart = 0;
		while ($objectStart < strlen($pack)) {
			$hash = "";
			$sizeString = "";
			$objectStart = $this->readTill($pack, $hash, $objectStart, " ");
			$objectStart = $this->readTill($pack, $sizeString, $objectStart, "\0");
			$size = (int)$sizeString;
			$this->writeFile($hash, substr($pack, $objectStart, $size));
			$objectStart += $size;
		}
  
		// update tip
		$currentTip = $this->repository->getBranchTip($branch);
		if ($currentTip == "")
			return $this->repository->setBranchTip($branch, $endCommit);

		// check if all commit objects are in place
		if (!$this-isAncestorCommit($endCommit, $currentTip))
			return false;
		// TODO also check if all blobs for the new commits are in place
		return $this->repository->setBranchTip($branch, $endCommit);
	}

	private function readTill($in, &$out, $start, $stopChar)
	{
		$pos = $start;
		while ($pos < strlen($in) && $in[$pos] != $stopChar) {
			$out = $out.$in[$pos];
			$pos++;
		}
		$pos++;
		return $pos;
	}

	private function isAncestorCommit($child, $ancestor) {
		$handledCommits = array();
		// list of unhandled commits
		$commits[] = $child;
		while (true) {
			$currentCommit = array_pop($commits);
			if ($currentCommit == NULL)
				break;
			if ($currentCommit == $ancestor)
				true;
			if (in_array($currentCommit, $handledCommits))
				continue;
			$handledCommits[] = $currentCommit;

			$commitObject = $this->repository->getObject($currentCommit);
			$commits[] = $commitObject->parents;
		}

		return false;
	}

	private function writeFile($hashHex, $data)
	{
		$path = sprintf('%s/objects/%s/%s', $this->repository->dir, substr($hashHex, 0, 2), substr($hashHex, 2));
		if (file_exists($path))
			return false;
		$dir = dirname($path);
		if (!is_dir($dir))
			mkdir(dirname($path), 0770);
		$f = fopen($path, 'ab');
		flock($f, LOCK_EX);
		ftruncate($f, 0);
		fwrite($f, $data);
		fclose($f);
		return true;
    }
   
	private function packObjects($objects) {
        $pack = '';
		foreach ($objects as $object) {
			list($type, $data) = $this->repository->getRawObject($object);
			$blob = Git::getTypeName($type).' '.strlen($data)."\0".$data;
			$blob = gzcompress($blob);
			$blob = sha1_hex($object).' '.strlen($blob)."\0".$blob;
			$pack = $pack.$blob;
		}
		return $pack;
	}

	private function listTreeObjects($treeName, &$objects) {
		if (!in_array($treeName, $objects))
			$objects[] = $treeName;
		$treesQueue = array();
		$treesQueue[] = $treeName;
		while (true) {
			$currentTree = array_pop($treesQueue);
			if ($currentTree == NULL)
				break;
			$treeObject = $this->repository->getObject($currentTree);
			foreach ($treeObject->nodes as $node)
			{
				if (!in_array($node->object, $objects))
					$objects[] = $node->object;
				if ($node->is_dir)
					$treesQueue[] = $node->object;
			}
		}
		#debug
		/*echo "List:<br>";
		foreach ($objects as $object)
			$this->printObject($object);*/
		return true;
    }

	private function findMissingObjects($listOld, $listNew) {
		$missing = array();

		sort($listOld);
		sort($listNew);

		$a = $b = 0;
		while ($a < count($listOld) || $b < count($listNew))
		{
			if ($a < count($listOld) && $b < count($listNew))
				$cmp = strcmp($listOld[$a], $listNew[$b]);
			else
				$cmp = 0;
			if ($b >= count($listNew) || $cmp < 0)
			{
				$a++;
			}
			else if ($a >= count($listOld) || $cmp > 0)
			{
				$missing[] = $listNew[$b];
				$b++;
			}
			else
			{
				$a++;
				$b++;
			}
		}

		return $missing;
    }

    /*! Collect all ancestors including the start $commit (binary SHA1).
    */
    private function collectAncestorCommits($commit) {
        $handledCommits = array();
        // list of unhandled commits
        $commits[] = $commit;
        while (true) {
            $currentCommit = array_pop($commits);
            if ($currentCommit == NULL)
                break;
            if (in_array($currentCommit, $handledCommits))
                continue;
            $handledCommits[] = $currentCommit;

            $commitObject = $this->repository->getObject($currentCommit);
            $commits[] = $commitObject->parents;
        }

        return $handledCommits;
    }

    // takes (binary SHA1)
	private function collectMissingBlobs($commitStop, $commitLast, $type = -1) {
		$commits = array();
		$newObjects = array();
		$commits[] = $commitLast;
		$stopAncestorCommits = array();
		$stopAncestorsCalculated = false;
		while (count($commits) > 0) {
			$currentCommit = array_pop($commits);
			if ($currentCommit == $commitStop)
				continue;
			if (in_array($currentCommit, $newObjects))
				continue;
			$newObjects[] = $currentCommit;

			// collect tree objects
			$commitObject = $this->repository->getObject($currentCommit);
			$this->listTreeObjects($commitObject->tree, $newObjects);

			$parents = $commitObject->parents;
            if (!$stopAncestorsCalculated && count($parents) > 1) {
                $stopAncestorCommits = $this->collectAncestorCommits($commitStop);
                $stopAncestorsCalculated = true;
            }
			foreach ($parents as $parent) {
				if (!in_array($parent, $stopAncestorCommits))
					$commits[] = $parent;
			}
		}

		// get stop commit object tree
		$stopCommitObjects = array();
		if ($commitStop != "") {
			$stopCommitObject = $this->repository->getObject($commitStop);
			if ($stopCommitObject === NULL)
				return array();
			
			$this->listTreeObjects($stopCommitObject->tree, $stopCommitObjects);
		}

		// calculate the missing objects
		return $this->findMissingObjects($stopCommitObjects, $newObjects);
	}
    /*
    // takes (binary SHA1)
	private function collectMissingBlobs($commitStop, $commitLast, $type = -1) {
		$blobs = array();
		$handledCommits = array();
		$commits = array();
        // list of unhandled commits
		$commits[] = $commitLast;
        // we don't have to calculate the ancestors till we encountered the first commit with more than one parent
        $stopAncestorCommits = array();
        $stopAncestorsCalculated = false;
		while (true) {
			$currentCommit = array_pop($commits);
			if ($currentCommit == NULL)
				break;
			if ($currentCommit == $commitStop)
				continue;
			if (in_array($currentCommit, $handledCommits))
				continue;
            if (in_array($currentCommit, $stopAncestorCommits))
                continue;
			$handledCommits[] = $currentCommit;

			$commitObject = $this->repository->getObject($currentCommit);
			$parents = $commitObject->parents;
            if (!$stopAncestorsCalculated && count($parents) > 1) {
                $stopAncestorCommits = collectAncestorCommits($commitStop);
                $stopAncestorsCalculated = true;
            }
			foreach ($parents as $parent) {
				$parentObject = $this->repository->getObject($parent);
				
				$diffs = $this->findMissingObjects($this->listTreeOjects($parentObject->tree),
					$this->listTreeOjects($commitObject->tree));
				foreach ($diffs as $diff) {
					$object = $this->repository->getObject($diff);
					if ($type <= 0 || $object->getType() == $type) {
						if (!in_array($diff, $blobs)) {
							$blobs[] = $diff;
						}
					}
				}
				$commits[] = $parent;
			}
		}
		foreach ($handledCommits as $handledCommit)
			$blobs[] = $handledCommit;

		return $blobs;
	}
	*/
}

?>
