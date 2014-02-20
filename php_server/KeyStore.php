<?PHP

include_once 'UserData.php';

class KeyStore extends UserData {
	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);
	}

	public function readAsymmetricKey($keyId, &$certificate, &$publicKey) {
		$ok = $this->read($keyId."/certificate", $certificate);
		if (!$ok)
			return false;
		$ok = $this->read($keyId."/public_key", $publicKey);
		if (!$ok)
			return false;
		return true;
	}

}

?>
