import morgan from "morgan";
import bodyParser from "body-parser";
import cors from "cors";
import express from "express";
import textRoute from "./controllers/textController.js";
import configRoute from "./controllers/configController.js";

const controller = {}
controller.config = (app)=>{

    app.use(bodyParser.json());
    app.use(express.json());
    app.use(bodyParser.urlencoded({ extended: true }));
    app.use(morgan("short"));
    app.use(cors())
}
controller.start = (app)=>{
    app.use("/text",textRoute);
    app.use("/config",configRoute);
}
export default controller;