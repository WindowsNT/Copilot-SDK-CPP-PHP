<?php

class Copilot {
    private $proc;
    private $model;
    private $token;
    private $fp;
    private $next_attachments = array();

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

            usleep(100000); // Sleep for 100ms to avoid busy waiting
        }
        return str_replace("<<END>>","",$out);
    }

    public function add_attachment($path) {
        $this->next_attachments[] = array("path" => $path,"type" => "file");
    }

    public function build_prompt($prompt)
    {
        $msg = json_encode(["model"=> $this->model,"prompt"=>$prompt,"token"=>$this->token,"attachments"=>$this->next_attachments]);
        $msg .= "\n";
        return $msg;
    }


    public function ask($prompt) {
    $msg = $this->build_prompt($prompt);
    $msg .= "\n";
    $this->next_attachments = array(); // Clear attachments after sending
    return $this->send($msg);
}


}