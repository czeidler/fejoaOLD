<?php

include_once 'Profile.php';


class Session {
	private $database;
	private $profile = null;

	private function __construct() {
		$this->database = null;
	}
    
	public static function get() {
		static $sSession = null;
		if ($sSession === NULL)
			$sSession = new Session();
		return $sSession;
	}

	public function clear() {
		$this->setSignatureToken("");
		$this->setLoginUser("");
		$this->setServerUser("");
		$this->setUserRoles(array());
		$this->profile = null;
		$this->database = null;
	}

	public function getDatabase() {
		if ($this->database !== null)
			return $this->database;
		$serverUser = $this->getServerUser();
		if ($serverUser == "")
			return null;
		$databasePath = $serverUser."/.git";
		if (!file_exists($databasePath))
			return null;
		return new GitDatabase($databasePath);
	}

	public function getProfile() {
		if ($this->profile !== null)
			return $this->profile;
		$database = $this->getDatabase();
		if ($database === null)
			return null;
		$this->profile = new Profile($database, "profile", "");
		return $this->profile;
	}

	public function getMainUserIdentity() {
		$profile = $this->getProfile();
		if ($profile === null)
			return null;
		return $profile->getUserIdentityAt(0);
	}

	public function getUserDir() {
		return $_SESSION['server_user'];
	}

	public function setSignatureToken($token) {
		$_SESSION['sign_token'] = $token;
	}

	public function getSignatureToken() {
		return $_SESSION['sign_token'];
	}

	public function setLoginUser($user) {
		$_SESSION['login_user'] = $user;
	}
	
	public function getLoginUser() {
		return $_SESSION['login_user'];
	}

	public function setServerUser($user) {
		$_SESSION['server_user'] = $user;
	}

	public function getServerUser() {
		if (!isset($_SESSION['server_user']))
			return "";
		return $_SESSION['server_user'];
	}

	public function setUserRoles($roles) {
		$_SESSION['user_roles'] = $roles;
	}
	
	public function getUserRoles() {
		if (!isset($_SESSION['user_roles']))
			return array();
		return $_SESSION['user_roles'];
	}
}

?>
