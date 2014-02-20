<?PHP

class UserData {
	protected $database;
	protected $branch;
	protected $baseDirectory;

	public function __construct($database, $branch, $baseDirectory) {
		$this->database = $database;
		$this->branch = $branch;
		$this->baseDirectory = $baseDirectory;
	}

	public function getDatabase() {
		return $this->database;
	}

	public function getBranch() {
		return $this->branch;
	}

	public function getDirectory() {
		return $this->baseDirectory;
	}

	public function setDirectory($directory) {
		$this->baseDirectory = $directory;
	}

	public function write($path, $data) {
		$this->database->write($this->branch, $this->prependBaseDir($path), $data);
		return true;
	}

	public function read($path, &$data) {
		$blob = $this->database->readBlobContent($this->branch, $this->prependBaseDir($path));
		if ($blob === null)
			return false;
		$data = $blob;
		return true;
	}

	public function listDirectories($path) {
		return $this->database->listDirectories($this->branch, $this->prependBaseDir($path));
	}

	public function commit() {
		return $this->database->commit($this->branch);
	}

	private function prependBaseDir($path) {
		if ($this->baseDirectory == "")
			return $path;
		return $this->baseDirectory."/".$path;
	}
}

?>
