<?php

include_once 'KeyStore.php';
include_once 'Mailbox.php';
include_once 'UserData.php';
include_once 'UserIdentity.php';


class Profile extends UserData {
	private $userIdentities = array();

	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);

		$ids = $this->listDirectories("user_identities");
		foreach ($ids as $identityId) {
			$branch;
			$baseDir;
			$result = $this->read("user_identities/".$identityId."/database_branch", $branch);
			if (!$result)
				continue;
			$result = $this->read("user_identities/".$identityId."/database_base_dir", $baseDir);
			if (!$result)
				continue;
			$identity = new UserIdentity($this, $branch, $baseDir);
			$result = $identity->open();
			if (!$result)
				continue;
			$this->userIdentities[] = $identity;
		}
	}

	public function getUserIdentityAt($index) {
		if ($index >= count($this->userIdentities))
			return null;
		return $this->userIdentities[$index];
	}

	public function getUserIdentityKeyStore($userIdentity) {
		$keyStoreId = $userIdentity->getKeyStoreId();
		$keyStoreIds = $this->listDirectories("key_stores");
		if (!in_array($keyStoreId, $keyStoreIds))
			return null;
		$branch;
		$baseDir;
		$result = $this->read("key_stores/".$keyStoreId."/database_branch", $branch);
		if (!$result)
			return null;
		$result = $this->read("key_stores/".$keyStoreId."/database_base_dir", $baseDir);
		if (!$result)
			return null;
		$keyStore = new KeyStore($this->getDatabase(), $branch, $baseDir);
		return $keyStore;
	}

	public function getMainMailbox() {
		$branch = "";
		$baseDir = "";
		$maiboxUid;
		$result = $this->read("main_mailbox", $maiboxUid);
		if (!$result)
			return null;
		$result = $this->read("mailboxes/".$maiboxUid."/database_branch", $branch);
		if (!$result)
			return null;
		$result = $this->read("mailboxes/".$maiboxUid."/database_base_dir", $baseDir);
		if (!$result)
			return null;

		// TODO: find the right user identity in case there are more than one!
		$userIdentity = $this->getUserIdentityAt(0);
		return new Mailbox($userIdentity, $branch, $baseDir);
	}
}

?>
