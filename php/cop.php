<?php

// Enable all errors
error_reporting(E_ALL);
ini_set('display_errors', 1);


class COPILOT_SESSION_PARAMETERS
{
	public $Streaming = true;
	public $reasoning_effort = "";
	public $system_message = "";
	public $skill_dirs = array();
	public $disabled_skills = array();
};


class Copilot {

    private $pid = 0;
    private $fp = 0;
    private $next_id = 1;
    private $response_buffer = "";

    private function ExtractResponsesFromBuffer()
    {
        $responses = array();

        // Buffer is this format
        // Content-Length: xxx\r\n\r\n<json message>
        while (true) {
            $pos = strpos($this->response_buffer, "\r\n\r\n");
            if ($pos === false) break;

            $header = substr($this->response_buffer, 0, $pos);
            $body = substr($this->response_buffer, $pos + 4);

            // Parse header for content length
            if (preg_match("/Content-Length: (\d+)/", $header, $matches)) {
                $content_length = (int)$matches[1];
                if (strlen($body) >= $content_length) {
                    $json_msg = substr($body, 0, $content_length);
                    $responses[] = json_decode($json_msg, true);
                    // Remove this message from buffer
                    $this->response_buffer = substr($body, $content_length);
                } else {
                    break; // Wait for more data
                }
            } else {
                // Invalid header, discard
                $this->response_buffer = substr($this->response_buffer, $pos + 4);
            }
        }
        return $responses;
    }
    
    private function next()
    {
        return $this->next_id++;
    }

    

    private function sendmessage($msg,$waittype,$timeout = 60) 
    {
        // Must also send content length header
        $jsonmsg = json_encode($msg);
        $msg2 = "Content-Length: " . strlen($jsonmsg) . "\r\n\r\n" . $jsonmsg;
        fwrite($this->fp, $msg2);

        if ($waittype == 3)
            return true; // Don't wait for response
        $all_type_1 = array();

        $start = time();
        for(;;)
            {
            if (time() - $start > $timeout)
                {
                    return array();
                }

                usleep(100000); // Sleep for 100ms to prevent CPU hogging
                // Socket terminated?
                if (feof($this->fp)) {
                    return array();
                }

                
                $read = [$this->fp];
                $write = null;
                $except = null;

                $data = "";
                if (stream_select($read,$write,$except,5))
                    $data = fread($this->fp,8192);
                
               // read from socket
//                $data = fread($this->fp, 8192);
                if ($data === false || strlen($data) == 0) {
                    continue;
                }

                // Append to buffer and extract complete responses
                $this->response_buffer .= $data;
                $responses = $this->ExtractResponsesFromBuffer();

                // Log responses to /var/log/copilot.log
/*                foreach ($responses as $response) {
                    file_put_contents("/backup/coplog/copilot.log", date("Y-m-d H:i:s") . " - Received: " . json_encode($response) . "\n", FILE_APPEND);
                }
*/

                if ($waittype == 1 || $waittype == 2)
                    {
                        // Wait for "session.idle" event for this session
                        $all_type_1 = array_merge($all_type_1, $responses);
                        foreach ($responses as $response)
                            {
                                // See if there is anything to request permission
                                if (isset($response["method"]) && $response["method"] == "permission.request")
                                {
                                    // find id
                                    $id = isset($response["id"]) ? $response["id"] : 0;
                                    // Answer immediately 
                                    $j = array();
                                    $j["jsonrpc"] = "2.0";
                                    if ($id)
                                        $j["id"] = $id;
                                    $j["result"] = array();
                                    $j["result"]["result"] = array();
                                    $j["result"]["result"]["kind"] = "approved";
                                    $this->sendmessage($j,3);
                                    continue;
                                }
                                if (isset($response["method"]) && $response["method"] == "session.event" && isset($response["params"]["event"]["type"]) && $response["params"]["event"]["type"] == "permission.requested") 
                                {
                                    // find id
                                    $id = isset($response["id"]) ? $response["id"] : 0;
                                    // Answer immediately 
                                    $j = array();
                                    $j["jsonrpc"] = "2.0";
                                    if ($id)
                                        $j["id"] = $id;
                                    $j["result"] = array();
                                    $j["result"]["result"] = array();
                                    $j["result"]["result"]["kind"] = "approved";
                                    $this->sendmessage($j,3);
                                    continue;
                                }



                                // if method is session.event and params.event.type is session.idle and params.sessionId matches the session we are waiting for, then return this response
                                if (isset($response["method"]) && $response["method"] == "session.event" && isset($response["params"]["event"]["type"]) && $response["params"]["event"]["type"] == "session.idle") 
                                    {
                                        if ($waittype == 2)
                                            {
                                                foreach ($all_type_1 as $msg)
                                                    {
                                                        // if is session.event and params.event.type is session.partial_response, then print the content of params.event.partial_response.content
                                                        if (isset($msg["method"]) && $msg["method"] == "session.event" && isset($msg["params"]["event"]["type"]) && $msg["params"]["event"]["type"] == "assistant.message") 
                                                            {
                                                                return $msg["params"]["event"]["data"]["content"];
                                                            }
                                                    }
                                                return "";
                                            }
                                        return $all_type_1;
                                    }
                            }
                    }

                if ($waittype == 0)
                {
                    // find response with matching id
                    foreach ($responses as $response) 
                        {
                        if (isset($response["id"]) && $response["id"] == $msg["id"]) 
                            {
                            fflush($this->fp);
                            return $response;
                        }
                    }   
                }
            }

            // flush socket
            fflush($this->fp);
            
    }

    public function Ping()
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "ping";
        // Send to socket
        return $this->sendmessage($r,0);
    }

    public function AuthStatus()
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "auth.getStatus";
        // Send to socket
        return $this->sendmessage($r,0);
    }

    public function Quota()
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "account.getQuota";
        // Send to socket
        return $this->sendmessage($r,0);
    }

    public function ModelList()
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "models.list";
        // Send to socket
        return $this->sendmessage($r,0);
    }

    public function Sessions()
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "session.list";
        // Send to socket
        return $this->sendmessage($r,0);
    }

    public function CreateSession($modelid,$params,$return_id = false)
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "session.create";
        $r["requestPermission"] = true;
        $r["envValueMode"] = "direct";
        $r["streaming"] = $params->Streaming;
        $r["params"] = array(
            "model" => $modelid,
            "system_message" => $params->system_message,
            "reasoning_effort" => $params->reasoning_effort,
            );
        // Send to socket
        $a = $this->sendmessage($r,0);
        if ($return_id)
            {
                return $a["result"]["sessionId"];
            }
        return $a;
    }

    public function Prompt($sessionid,$message,$return_only_response = false)
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "session.send";
        $r["params"] = array(
            "sessionId" => $sessionid,
            "prompt" => $message,
            );
        // Send to socket
        return $this->sendmessage($r,$return_only_response ? 2 : 1);
    }

    public function EndSession($sessionid,$delete_files)
    {
        $r = array();
        $r["jsonrpc"] = "2.0";
        $r["id"] = $this->next();
        $r["method"] = "session.destroy";
        if ($delete_files)
            $r["method"] = "session.delete";
        $r["params"] = array(
            "sessionId" => $sessionid,
            );
        return $this->sendmessage($r,0);
    }

    public function __construct($token,$cli_path = "/usr/local/bin/copilot",$port = 8765) 
    { 
        if (strlen($token) && strlen($cli_path)) 
        {
            // Start copilot binary with these parameters
            // --no-auto-update --auth-token-env token_env --acp --port %d --log-level info --headless
            
            // Create environment variable for token
            putenv("COPILOT_TOKEN=$token");
            // Start the copilot process
            $cmd = sprintf("%s --no-auto-update --auth-token-env COPILOT_TOKEN --acp --port %d --log-level info --headless > /dev/null 2>&1 & echo $!",$cli_path,$port);
            $output = array();
            exec($cmd,$output);
            $this->pid = (int)$output[0];
            // Wait a bit for the process to start and listen on the socket
            sleep(2);
        }

        // Connect to 127.0.0.1:port and keep the socket open for sending messages
        $this->fp = fsockopen("127.0.0.1",$port,$errno,$errstr,5);
        if(!$this->fp) throw new Exception($errstr);
        stream_set_blocking($this->fp, false);
    }

    // Destructor to ensure the process is killed when the object is destroyed
    public function __destruct() 
    {
        if ($this->fp) {
            // Close the socket connection
            fclose($this->fp);
        }
        if ($this->pid) {
            // Kill the process
            exec("kill " . $this->pid);
        }
    }
}



