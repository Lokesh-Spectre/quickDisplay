import express from "express";
import {server} from "./constants.js";
import controller from "./controllers.js";
import mqtt_services from "./mqtt_services.js";
import timeJob from "./jobs/timeJob.js";

const app = express();

controller.config(app);
controller.start(app);

mqtt_services();
timeJob();
app.get("/", async (req, res) => {
    return res.status(200).send({ status: "success", message: "This is backend for quickDisplay." })
});


app.listen(server.PORT, (err) => {
    if (err) {
        console.log(`***${err}`)
    } else {
        console.log(`Server started at ${server.PORT}`)
    }
})

