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
		$this->channelUid = $xml->getAttribute("channel");
		return true;
	}

	public function finished() {
		$profile = Session::get()->getProfile();
		if ($profile === null)
			throw new exception("unable to get profile");

		$mailbox = $profile->getMainMailbox();
		if ($mailbox === null)
			throw new exception("unable to get mailbox");

		$ok = true;
		if ($this->channelStanzaHandler->hasBeenHandled())
			$ok = $mailbox->addChannel($this->channelUid, $this->messageChannel);
		if ($ok && $this->channelInfoStanzaHandler->hasBeenHandled())
			$ok = $mailbox->addChannelInfo($this->channelUid, $this->channelInfo);
		if ($ok)
			$ok = $mailbox->addMessage($this->channelUid, $this->message);
		if ($ok)
			$ok = $mailbox->commit() !== null;
	
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza(MessageConst::$kMessageStanza);
		if ($ok)
			$stanza->addAttribute("status", "message_received");
		else {
			$stanza->addAttribute("status", "declined");
			$stanza->addAttribute("error", $mailbox->getLastErrorMessage());
		}
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}


?>
