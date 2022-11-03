CREATE TABLE `IoT`.`Data` ( 
`ID` INT NOT NULL AUTO_INCREMENT , 
`DeviceID` INT NOT NULL , 
`LogID` INT NOT NULL , 
`FechaHoraData` DATETIME NOT NULL , 
`FechaHoraRecepcion` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP , 
`Codigo` INT NOT NULL , 
`Data` VARCHAR(150) NOT NULL , 
`Version` INT NOT NULL , 
PRIMARY KEY (`ID`), 
INDEX `DeviceID_Indice` (`DeviceID`), 
INDEX `Codigo_Indice` (`Codigo`)) 
ENGINE = InnoDB;
