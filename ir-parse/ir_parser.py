import os
import pandas as pd

def get_code(file_name, in_decimal=False):
    """
    Converts a csv of time, channel pairs to the code value to create the signal that created the csv data

    Initial columns are "Time[s]" and " Channel %d"
    Each row indicates the time at which the value of the channel switched to the value indicated in the channel column
    For example, the row [1.5, 0] indicates that the channel switched off at time 1.5s

    Value of 0 in the "on" column corresponds to activation (activation-low)
    """
    df = pd.read_csv(file_name)
    df.columns = ["time", "on"]
    df["time"] = df.time.astype(float)

    prev_time = df.time.iloc[:-1]
    prev_time.index += 1
    curr_time = df.time.iloc[1:]
    df["diff"] = (curr_time - prev_time) * 1000
    df = df[["diff", "on"]].dropna(subset=["diff"])

    df = df[df.on == 0]
    df["value"] = df["diff"] > 1
    binary = "".join(df.value.astype(int).astype(str))

    if in_decimal:
        return int(binary, 2)
    else:
        return binary
    

if __name__ == "__main__":
    file_name = input("File name: ")
    if not file_name:
        for file_name in os.listdir("./signal-data"):
            full_file_name = "./signal-data/" + file_name
            print(f"{file_name}: ", get_code(full_file_name))
    else:
        full_file_name = "./signal-data/" + file_name
        print("Code: ", get_code(full_file_name))