<?php
require_once "copilot.php";
$copilot = new Copilot(); 
//$q = "What is the capital of france?";
//echo "Asking: $q<br>";
//echo $copilot->ask($q) . "<br><br>";
echo $copilot->send("/models");
echo '<br><br>';
echo $copilot->send("/state");
echo '<br><br>';
echo $copilot->send("/authstate");
echo '<br><br>';
echo $copilot->ask("What is the capital of france?");
$copilot->kill();
