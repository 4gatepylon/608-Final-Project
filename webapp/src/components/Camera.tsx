import React from "react";
import { NavBar } from "./NavBar";

function Camera() {
  const camera = (
    <div className="bg-white h-20 rounded-lg">
      {/* <img
        src={
          "data:image/gif;base64, R0lGODlhAQABAIAAAP///////yH5BAEKAAEALAAAAAABAAEAAAICTAEAOw=="
        }
        className="object-contain"
        alt={"camera output"}
      /> */}

      <img
        src="https://www.w3schools.com/images/w3schools_green.jpg"
        alt="W3Schools.com"
      />
    </div>
  );

  return (
    <div>
      <NavBar />
      {camera}
    </div>
  );
}

export default Camera;
