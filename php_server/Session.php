<?php

include_once 'Profile.php';


class Session {
	private function __construct() {
	}
    
	public static function get() {
		static $sSession = null;
		if ($sSession === NULL)
			$sSession = new Session();
		return $sSession;
	}

	public function clear() {
		$this->setSignatureToken("");
		$this->setUserRoles(array());
	}

	public function getDatabase($user) {
		$databasePath = $user."/.git";
		if (!file_exists($databasePath))
			return null;
		return new GitDatabase($databasePath);
	}

	public function getProfile($user) {
		$database = $this->getDatabase($user);
		if ($database === null)
			return null;
		return new Profile($database, "profile", "");
	}

	public function getMainUserIdentity($user) {
		$profile = $this->getProfile($user);
		if ($profile === null)
			return null;
		return $profile->getUserIdentityAt(0);
	}

	public function getAccountUser() {
		if (!isset($_SESSION['account_user']))
			return "";
		return $_SESSION['account_user'];
	}
	
	public function setAccountUser($user) {
		$_SESSION['account_user'] = $user;
	}

	// the purpose has the format: serverUserUid:loginUserID
	// server user is the user who has the account on the server
	// login user is the user who does the request
	public function setSignatureToken($purpose, $token) {
		$_SESSION[$purpose] = $token;
	}

	public function getSignatureToken($purpose) {
		return $_SESSION[$purpose];
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
