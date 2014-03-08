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
		$this->setLoginUser("");
		$this->setServerUser("");
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

	public function setSignatureToken($token) {
		$_SESSION['sign_token'] = $token;
	}

	public function getSignatureToken() {
		return $_SESSION['sign_token'];
	}

	// login user is a user who, for example, post a message to a server user account
	public function setLoginUser($user) {
		$_SESSION['login_user'] = $user;
	}
	
	public function getLoginUser() {
		return $_SESSION['login_user'];
	}

	// server user is the user who has an account on the server
	public function setLoginServerUser($user) {
		$_SESSION['login_server_user'] = $user;
	}

	public function getLoginServerUser() {
		if (!isset($_SESSION['login_server_user']))
			return "";
		return $_SESSION['login_server_user'];
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
