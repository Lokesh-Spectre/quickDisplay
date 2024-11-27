import os from "os";
import path from "path";

export const homeDir =path.join(os.homedir(),".local/share/cosmohub-dashboard");
// export const servicesFolder = `${homeDir}/services`;
export const services = [];
export const server = {
    url:"http://localhost:3000",
    PORT:3000,
    mqtt_broker_url:"mqtt://cosmohub:1883"
}

export default {homeDir, services,server }