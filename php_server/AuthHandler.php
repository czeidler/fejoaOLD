<?php

include_once 'Session.php';
include_once 'XMLProtocol.php';


class AuthConst {
	static public $kAuthStanza = "auth";
	static public $kAuthSignedStanza = "auth_signed"; 
}


class AccountAuthStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $authType;
	private $loginUser;
	private $serverUser;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(AuthConst::$kAuthStanza);
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		$this->authType = $xml->getAttribute("type");
		$this->loginUser = $xml->getAttribute("loginUser");
		$this->serverUser = $xml->getAttribute("serverUser");
		if ($this->authType == "" || $this->loginUser == "" || $this->loginUser == "")
			return false;
		return true;
	}

	public function finished() {
		if (strcmp($this->authType, "signature"))
			$this->inStreamReader->appendResponse(IqErrorOutStanza::makeErrorMessage("Unsupported authentication type."));

		$signToken = "rand".rand()."time".time();
		Session::get()->setSignatureToken($this->serverUser.":".$this->loginUser, $signToken);

		// Check if the user has a change to login, i.e., if we know him
		$userIdentity = Session::get()->getMainUserIdentity($this->serverUser);
		// if $userIdentity is null the database is invalid give the user a change to upload a profile
		if ($userIdentity != null) {
			$contact = $userIdentity->findContact($this->loginUser);
			if ($userIdentity->getMyself()->getUid() != $this->loginUser && $contact === null) {
				// produce output
				$outStream = new ProtocolOutStream();
				$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
				$stanza = new OutStanza(AuthConst::$kAuthStanza);
				$stanza->addAttribute("status", "i_dont_know_you");
				$outStream->pushChildStanza($stanza);

				$this->inStreamReader->appendResponse($outStream->flush());
				return;
			}
		}

		
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(AuthConst::$kAuthStanza);
		$stanza->addAttribute("status", "sign_this_token");
		$stanza->addAttribute("sign_token", $signToken);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}


class AccountAuthSignedStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $errorMessage = "";
	private $serverUser = "";
	private $loginUser = "";
	private $signature;

	private function getPurpose() {
		return $this->serverUser.":".$this->loginUser;
	}

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(AuthConst::$kAuthSignedStanza);
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		$this->signature = $xml->getAttribute("signature");
		if ($this->signature == "")
			return false;
		$this->signature = url_decode($this->signature);

		$this->serverUser = $xml->getAttribute("serverUser");
		if ($this->serverUser == "")
			return false;
		$this->loginUser = $xml->getAttribute("loginUser");
		if ($this->loginUser == "")
			return false;
			
		return true;
	}

	private function removeDuplicates($array){
		$cleanedArray = array();
		foreach($array as $key=>$value)
			$cleanedArray[$value] = 1;
		return array_keys($cleanedArray);
	}

	public function finished() {
		$userIdentity = Session::get()->getMainUserIdentity($this->serverUser);

		$status = false;
		$roles = Session::get()->getUserRoles();
		if ($userIdentity != null) {
			$loginUser = $this->loginUser;
			$myself = $userIdentity->getMyself();
			if ($myself->getUid() == $loginUser) {
				$status = $this->accountLogin($myself, $roles);
			} else {
				$contact = $userIdentity->findContact($loginUser);
				if ($contact !== null)
					$status = $this->userLogin($contact, $roles);
				else
					$this->errorMessage = "can't find user: ".$loginUser;
			}
		} else {
			$status = $this->setupLogin($roles);
		}
		// cleanup from double logins.
		//TODO: check earlier if verification was neccessary? so that we don't need to cleanup here
		$roles = $this->removeDuplicates($roles);
		Session::get()->setUserRoles($roles);

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(AuthConst::$kAuthSignedStanza);
		$statusMessage;
		if ($status)
			$statusMessage = "ok";
		else
			$statusMessage = "denied";
		$stanza->addAttribute("status", $statusMessage);
		$stanza->addAttribute("message", $this->errorMessage);
		$outStream->pushChildStanza($stanza);
		$roles = Session::get()->getUserRoles();
		$firstRole = true;
		foreach ($roles as $role) {
			$stanza = new OutStanza("role");
			$stanza->setText($role);
			if ($firstRole) {
				$outStream->pushChildStanza($stanza);
				$firstRole = false;
			} else
				$outStream->pushStanza($stanza);
		}
		$outStream->cdDotDot();
		$this->inStreamReader->appendResponse($outStream->flush());
	}

	private function setupLogin(&$roles) {
		$signatureFileName = $this->serverUser."/signature.pup";
		$publickey = "";
		if (file_exists($signatureFileName))
			$publickey = file_get_contents($signatureFileName);

		$signatureVerifier = new SignatureVerifier($publickey);
		if (!$signatureVerifier->verify(Session::get()->getSignatureToken($this->getPurpose()),
			$this->signature)) {
			$this->errorMessage = "can't verify setup login";
			return false;
		}
		Session::get()->setAccountUser($this->serverUser);
		$roles[] =  "account";
		return true;
	}

	private function accountLogin($contact, &$roles) {
		if (!$contact->verify($contact->getMainKeyId(),
			Session::get()->getSignatureToken($this->getPurpose()), $this->signature)) {
			$this->errorMessage = "can't verify account login";
			return false;
		}
		Session::get()->setAccountUser($this->serverUser);
		$roles[] =  "account";
		return true;
	}

	private function userLogin($contact, &$roles) {
		if (!$contact->verify($contact->getMainKeyId(),
			Session::get()->getSignatureToken($this->getPurpose()), $this->signature)) {
			$this->errorMessage = "can't verify user login";
			return false;
		}
		$loginServerUser = $this->serverUser;
		$roles[] = $loginServerUser.":contact_user";
		
		return true;
	}
}


class LogoutStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct("logout");
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		Session::get()->clear();

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza("logout");
		$stanza->addAttribute("status", "ok");
		$outStream->pushChildStanza($stanza);
		$this->inStreamReader->appendResponse($outStream->flush());

		return true;
	}
}
?>
