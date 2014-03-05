<?php

include_once 'Mailbox.php';
include_once 'XMLProtocol.php';


class MessageConst {
	static public $kPutMessageStanza = "put_message";
	static public $kMessageStanza = "message";
	static public $kChannelStanza = "channel";
	static public $kChannelInfoStanza = "channel_info";
};


class SignedPackageStanzaHandler extends InStanzaHandler {
	private $signedPackage;
	
	public function __construct($signedPackage, $stanzaName) {
		InStanzaHandler::__construct($stanzaName);
		$this->signedPackage = $signedPackage;
	}
	
	public function handleStanza($xml) {
		$this->signedPackage->uid = $xml->getAttribute("uid");
		$this->signedPackage->sender = $xml->getAttribute("sender");
		$this->signedPackage->signatureKey = $xml->getAttribute("signatureKey");
		$this->signedPackage->signature = base64_decode($xml->getAttribute("signature"));
		$this->signedPackage->data = base64_decode($xml->readString());
		return true;
	}
};


class MessageStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $lastErrorMessage = "";

	private $receiver;
	private $channelUid;
	private $messageChannel;
	private $channelInfo;
	private $message;
	private $messageStanzaHandler;
	private $channelStanzaHandler;
	private $channelInfoStanzaHandler;
	
	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(MessageConst::$kPutMessageStanza);
		$this->inStreamReader = $inStreamReader;

		$this->message = new SignedPackage();
		$this->messageChannel = new SignedPackage();
		$this->channelInfo = new SignedPackage();
		
		$this->messageStanzaHandler = new SignedPackageStanzaHandler($this->message, MessageConst::$kMessageStanza);
		$this->channelStanzaHandler = new SignedPackageStanzaHandler($this->messageChannel, MessageConst::$kChannelStanza);
		$this->channelInfoStanzaHandler = new SignedPackageStanzaHandler($this->channelInfo, MessageConst::$kChannelInfoStanza);

		$this->addChild($this->messageStanzaHandler, false);
		// optional
		$this->addChild($this->channelStanzaHandler, true);
		$this->addChild($this->channelInfoStanzaHandler, true);
	}

	public function handleStanza($xml) {
		$this->receiver = $xml->getAttribute("server_user");
		if ($this->receiver == "")
			return false;
		
		$this->channelUid = $xml->getAttribute("channel");
		return true;
	}

	private function putMessage() {
		// login check, if not authenticated already return here
		$roles = Session::Get()->getUserRoles();
		if (!in_array($this->receiver.":contact_user", $roles)) {
			$this->lastErrorMessage = "not authenticated";
			return false;
		}

		$profile = Session::get()->getProfile($this->receiver);
		if ($profile === null) {
			$this->lastErrorMessage = "unable to get profile";
			return false;
		}
		$mailbox = $profile->getMainMailbox();
		if ($mailbox === null) {
			$this->lastErrorMessage = "unable to get mailbox";
			return false;
		}

		if ($this->channelStanzaHandler->hasBeenHandled()) {
			if (!$mailbox->addChannel($this->channelUid, $this->messageChannel)) {
				$this->lastErrorMessage = $mailbox->getLastErrorMessage();
				return false;
			}
		}
		if ($this->channelInfoStanzaHandler->hasBeenHandled()) {
			if (!$mailbox->addChannelInfo($this->channelUid, $this->channelInfo)) {
				$this->lastErrorMessage = $mailbox->getLastErrorMessage();
				return false;
			}
		}
		if (!$mailbox->addMessage($this->channelUid, $this->message)) {
			$this->lastErrorMessage = $mailbox->getLastErrorMessage();
			return false;
		}

		$ok = $mailbox->commit() !== null;
		return $ok;
	}

	public function finished() {
		$ok = $this->putMessage();

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza(MessageConst::$kMessageStanza);
		if ($ok)
			$stanza->addAttribute("status", "message_received");
		else {
			$stanza->addAttribute("status", "declined");
			$stanza->addAttribute("error", $this->lastErrorMessage);
		}
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}


?>
