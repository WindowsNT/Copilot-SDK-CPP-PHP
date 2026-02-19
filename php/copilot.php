<?php

class Copilot {
    private $proc;
    private $model;
    private $token;
    private $fp;

    public function __construct($model="gpt-4.1",$token = "",$port = 8765) { 
        $this->model = $model;
        $this->token = $token;
          $this->fp = fsockopen("127.0.0.1",$port,$errno,$errstr,5);
          if(!$this->fp) throw new Exception($errstr);
    }


    public function kill()
    {
        // Close $fp
        fclose($this->fp);
    }

    public function send($msg) {
        fwrite($this->fp, $msg."\n");
        $out = "";
        while(true) {
            $chunk = fread($this->fp,8192);
            if($chunk!==false) $out .= $chunk;
            if(str_contains($out,"<<END>>"))
                break;
            if ($chunk===false) break;
        }
        return str_replace("<<END>>","",$out);
    }

    public function ask($prompt) {
    $msg = json_encode(["model"=> $this->model,"prompt"=>$prompt,"token"=>$this->token]);
    $msg .= "\n";
    return $this->send($msg);
}


}