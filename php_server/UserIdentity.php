<?php

include_once 'Contact.php';
include_once 'UserData.php';


/*! The server side can't encrypt or decrypt data in the database so just derive from UserData. */
class UserIdentity extends UserData {
	private $profile;
	private $contacts = array();

	public function __construct($profile, $branch, $directory) {
		parent::__construct($profile->getDatabase(), $branch, $directory);
		$this->profile = $profile;
	}

	public function open() {
		$contactNames = $this->listDirectories("contacts");
		if ($contactNames === null)
			return true;
		foreach ($contactNames as $contactName) {
			$path = $this->getDirectory()."/contacts/".$contactName;
			$contact = new Contact($this, $path);
			$this->contacts[] = $contact;
        }
        return true;
	}

	public function getProfile() {
		return $this->profile;
	}

	public function getContacts() {
		return $this->contacts;
	}

	public function createContact($contactUid) {
		if ($contactUid == "")
			throw new exception("invalid contact uid");
		$contact = new Contact($this, $this->getDirectory()."/contacts/".$contactUid);
		$contact->setUid($contactUid);
		$this->contacts[] = $contact;
		return $contact;
	}
	
	public function findContact($uid) {
		foreach ($this->contacts as $contact) {
			if ($contact->getUid() == $uid)
				return $contact;
		}
		return null;
	}

	public function getUid() {
		$uid = "";
		$this->read("uid", $uid);
		return $uid;
	}

	public function getKeyStoreId() {
		$keyStoreId = "";
		$this->read("key_store_id", $keyStoreId);
		return $keyStoreId;
	}

	public function getMyself() {
		return new Contact($this, $this->getDirectory()."/myself");
	}
}

?>
