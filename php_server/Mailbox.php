<?php

include_once 'UserData.php';


class SignedPackage {
	public $uid;
	public $sender;
	public $signatureKey;
	public $signature;
	public $data;
}

class Mailbox extends UserData {
	private $lastErrorMessage;
	private $userIdentity;

	public function __construct($userIdentity, $branch, $directory) {
		parent::__construct($userIdentity->getDatabase(), $branch, $directory);

		$this->userIdentity = $userIdentity;
	}

	public function addChannel($channelId, $messageChannel) {
		if (!$this->verifyPackage($messageChannel))
			return false;
		
		if ($channelId != $messageChannel->uid) {
			$this->lastErrorMessage = "bad channel uid";
			return false;
		}

		$path = $this->pathForChannelId($messageChannel->uid);
		return $this->writePackage($messageChannel, $path);
	}

	public function addChannelInfo($channelId, $channelInfo) {
		$path = $this->dirForChannelId($channelId);
		$path = $path."/i/".$this->makeUidPath($channelInfo->uid);
		return $this->writePackage($channelInfo, $path);
	}

	public function addMessage($channelId, $message) {
		if (!$this->isValid($channelId, $message))
			return false;

		$path = $this->pathForMessageId($channelId, $message->uid);
		$path = $path."/m";
		return $this->writePackage($message, $path);
	}

	public function getLastErrorMessage() {
		return $this->lastErrorMessage;
	}

	public function hasChannel($channelUId) {
		$path = $this->pathForChannelId($channelUId);
		$data;
		$result = $this->read($path, $data);
		if (!$result)
			return false;
		return true;
	}

	public function hasMessage($channelId, $messageUid) {
		$path = $this->pathForMessageId($channelId, $messageUid);
		$data;
		$ok = $this->read($path, $data);
		if (!$ok)
			return false;
		return true;
	}

	private function pathForMessageId($channelId, $messageId) {
		$path = $this->dirForChannelId($channelId)."/".$this->makeUidPath($messageId);
		return $path;
	}

	private function pathForChannelId($channelId) {
		return $this->dirForChannelId($channelId)."/r";
	}
	
	private function dirForChannelId($channelId) {
		return $this->makeUidPath($channelId);
	}
	
	private function makeUidPath($uid) {
		$path = sprintf('%s/%s', substr($uid, 0, 2), substr($uid, 2));
		return $path;
	}

	private function isValid($channelUid, $message) {
		if ($channelUid === null || $channelUid == "") {
			$this->lastErrorMessage = "no message channel given";
			return false;
		}
		/*if ($this->hasMessage($channelUid, $message->uid)) {
			$this->lastErrorMessage = "message with same uid exist";
			return false;
		}

		if (!$this->hasChannel($channelUid)) {
			$this->lastErrorMessage = "channel not found";
			return false;
		}*/

		if (!$this->verifyPackage($message))
			return false;

		return true;
	}

	private function verifyPackage($package) {
		$signatureLength = unpack("N", $package->data)[1];
		$mainData = substr($package->data, $signatureLength + 4);
		$hash = hash('sha256', $mainData);

		$sender = $this->userIdentity->findContact($package->sender);
		if ($sender === null) {
			$this->lastErrorMessage = "sender unknown";
			return false;
		}
	
		$ok = $sender->verify($package->signatureKey, $hash, $package->signature);
		if (!$ok)
			$this->lastErrorMessage = "bad signature";
		return $ok;
	}

	private function writePackage($package, $path) {
		return $this->write($path, $package->data);
	}
}

?>
