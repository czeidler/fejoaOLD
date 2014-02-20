<?php

include_once './XMLProtocol.php';


class SyncPullStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $database;
	public function __construct($inStreamReader, $database) {
		InStanzaHandler::__construct("sync_pull");
		$this->inStreamReader = $inStreamReader;
		$this->database = $database;
	}

	public function getType() {
		return $this->type;
	}

	public function handleStanza($xml) {
		$branch = $xml->getAttribute("branch");
		$remoteTip = $xml->getAttribute("base");
		if ($branch === null || $remoteTip === null)
			return;
		if (isSHA1Hex($remoteTip))
			$remoteTip = sha1_bin($remoteTip);

		$packManager = new PackManager($this->database);
		$pack = "";
		try {
			$localTip = $this->database->getTip($branch);
			$pack = $packManager->exportPack($branch, $remoteTip, $localTip, -1);
		} catch (Exception $e) {
			$localTip = "";
		}

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza("sync_pull");
		$stanza->addAttribute("branch", $branch);
		$localTipHex = "";
		$remoteTipHex = "";
		if (strlen($remoteTip) == 20)
			$remoteTipHex = sha1_hex($remoteTip);
		$stanza->addAttribute("base", $remoteTipHex);
		if (strlen($localTip) == 20)
			$localTipHex = sha1_hex($localTip);
		$stanza->addAttribute("tip", $localTipHex);
		$outStream->pushChildStanza($stanza);
		
		$packStanza = new OutStanza("pack");
		$packStanza->setText(base64_encode($pack));
		$outStream->pushChildStanza($packStanza);
		
		$this->inStreamReader->appendResponse($outStream->flush());
		return true;
	}
}

class SyncPushPackHandler extends InStanzaHandler {
	private $pack;

	public function __construct() {
		InStanzaHandler::__construct("pack");
	}
	
	public function handleStanza($xml) {
		$this->pack = url_decode($xml->readString());
		return true;
	}
	
	public function getPack() {
		return $this->pack;
	}
}

class SyncPushStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $database;
	private $packHandler;
	
	private $branch;
	private $startCommit;
	private $lastCommit;

	public function __construct($inStreamReader, $database) {
		InStanzaHandler::__construct("sync_push");
		$this->inStreamReader = $inStreamReader;
		$this->database = $database;
	}

	public function handleStanza($xml) {
		$this->branch = $xml->getAttribute("branch");
		$this->startCommit = $xml->getAttribute("start_commit");
		$this->lastCommit = $xml->getAttribute("last_commit");
		$this->packHandler = new SyncPushPackHandler();
		$this->addChild($this->packHandler);
		return true;
	}
	
	public function finished() {
		$pack = $this->packHandler->getPack();
		
		$packManager = new PackManager($this->database);
		if (!$packManager->importPack($this->branch, $pack, $this->startCommit, $this->lastCommit)) {
			$this->inStreamReader->appendResponse(IqErrorOutStanza::makeErrorMessage("Push: unable to import pack."));
			return;
		}
		
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza("sync_push");
		$stanza->addAttribute("branch", $this->branch);
		$localTip = sha1_hex($this->database->getTip($this->branch));
		$stanza->addAttribute("tip", $localTip);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}

?>
