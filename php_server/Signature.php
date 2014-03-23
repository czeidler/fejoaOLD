<?php

set_include_path('phpseclib0.3.5');
include_once 'Crypt/RSA.php';
 
class SignatureVerifier {
	public $rsa;

	public function __construct($publickey) {
		$this->rsa = new Crypt_RSA();
		$this->rsa->setSignatureMode(CRYPT_RSA_SIGNATURE_PKCS1);
		$this->rsa->setHash('sha1');

		$this->rsa->loadKey($publickey);
	}

	public function verify($data, $signature) {
		return $this->rsa->verify($data, $signature);
	}
}
