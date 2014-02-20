<?php

include_once 'Signature.php';
include_once 'UserData.php';


class Contact extends UserData {
	public function __construct($userData, $directory) {
		parent::__construct($userData->getDatabase(), $userData->getBranch(), $directory);
	}

	public function setUid($uid) {
		$this->write("uid", $uid);
	}

	public function getUid() {
		$uid;
		$this->read("uid", $uid);
		return $uid;
	}

	public function verify($keyId, $data, $signature) {
		$certificate;
		$publicKey;
		$ok = $this->getKeySet($keyId, $certificate, $publicKey);
		if (!$ok)
			return false;
		$signatureVerifier = new SignatureVerifier($publicKey);
		return $signatureVerifier->verify($data, $signature);
	}

	public function getKeySet($keyId, &$certificate, &$publicKey) {
		$keyStoreType;
		$this->read("keystore_type", $keyStoreType);
		if ($keyStoreType == "private") {
			$profile = Session::get()->getProfile();
			$userIdentity = Session::get()->getMainUserIdentity();
			$keyStore = $profile->getUserIdentityKeyStore($userIdentity);
			$ok = $keyStore->readAsymmetricKey($keyId, $certificate, $publicKey);
			if (!$ok)
				return false;
		} else {
			$ok = $this->read("keys/".$keyId."/certificate", $certificate);
			if (!$ok)
				return false;
			$ok = $this->read("keys/".$keyId."/public_key", $publicKey);
			if (!$ok)
				return false;
		}
		return true;
	}

	public function addKeySet($keyId, $certificate, $publicKey) {
		$this->write("keys/".$keyId."/certificate", $certificate);
		$this->write("keys/".$keyId."/public_key", $publicKey);
	}

	public function setMainKeyId($mainKeyId) {
		$this->write("keys/main_key_id", $mainKeyId);
	}

	public function getMainKeyId() {
		$mainKeyId;
		$this->read("keys/main_key_id", $mainKeyId);
		return $mainKeyId;
	}

	public function addData($path, $data) {
		$this->write($path, $data);
	}

	public function getAddress() {
		$address;
		$this->read("address", $address);
		return $address;
	}

	public function setAddress($address) {
		$this->write("address", $address);
	}
}

?>
