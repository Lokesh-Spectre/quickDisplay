import express from "express";
import client from "../mqtt.js";
const router = express.Router();

router.post("/",(req,res)=>{
  const {text,speed,direction} = req.body;
  console.log("Sending text: "+text);
  client.publish("/cosmicForge/text",JSON.stringify({
    text,
    speed:speed??35,
    left: direction==="right"?0:1
  }))
  res.status(200).send();
});

export default router;