<?php

include_once 'XMLProtocol.php';


class WatchMessageConst {
	static public $kWatchStanza = "watch_branches";
	static public $kWatchItemsStanza = "branch";
};


class WatchItemsStanzaHandler extends InStanzaHandler {
	private $branchToTipMap = array();
	
	public function __construct() {
		InStanzaHandler::__construct(WatchMessageConst::$kWatchItemsStanza);
	}
	
	public function handleStanza($xml) {
		$this->branchToTipMap[$xml->getAttribute("branch")] = $xml->getAttribute("tip");
		return true;
	}

	public function getBranchToTipMap() {
		return $this->branchToTipMap;
	}
};


class WatchBranchesStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $watchItemsStanzaHandler;

	private $timeOut;
	private $pollInterval;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(WatchMessageConst::$kWatchStanza);
		
		$this->timeOut = 60 * 1; // x minutes
		$this->pollInterval = 1;

		$this->inStreamReader = $inStreamReader;
		$this->watchItemsStanzaHandler = new WatchItemsStanzaHandler();
		$this->addChild($this->watchItemsStanzaHandler);
	}

	public function handleStanza($xml) {
		return true;
	}

	public function finished() {
		$database = Session::get()->getDatabase(Session::get()->getAccountUser());
		if ($database === null)
			throw new exception("unable to get database");

		// allow other requests from the same client to get through, e.g., to post messages
		session_write_close();

		$updatedBranches = array();

		$status = "server_timeout";
		$startTime = time();
		$diff = 0;
		while ($diff < $this->timeOut) {
			$map = $this->watchItemsStanzaHandler->getBranchToTipMap();
			foreach ($map as $branch => $tip) {
				try {
					$currentTip = sha1_hex($database->getTip($branch));
				} catch (exception $e) {
					// branch is not there so trigger a sync by returning an invalid tip
					$currentTip = "DoesNotExist";
				}
				if ($currentTip != $tip)
					$updatedBranches[] = $branch;
			}
			if (count($updatedBranches) > 0) {
				$status = "update";
				break;
			}
			sleep($this->pollInterval);
			$diff = time() - $startTime;
		}

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza(WatchMessageConst::$kWatchStanza);
		$stanza->addAttribute("status", $status);
		$outStream->pushChildStanza($stanza);

		foreach ($updatedBranches as $branch) {
			$stanza = new OutStanza(WatchMessageConst::$kWatchItemsStanza);
			$stanza->addAttribute("branch", $branch);
			$outStream->pushChildStanza($stanza);
			$outStream->cdDotDot();
		}
		$this->inStreamReader->appendResponse($outStream->flush());

	}
}

?>
