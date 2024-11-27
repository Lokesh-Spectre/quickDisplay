import express from "express";
import client from "../mqtt.js";
const router = express.Router();
var light_level=0;
router.post("/",(req,res)=>{
  const {mode,light} = req.body;

  light_level = light?? light_level
  client.publish("/cosmicForge/config",JSON.stringify({
    mode:mode??"clock",
    light:light??light_level
  }))
  res.status(200).send();
});

export default router;