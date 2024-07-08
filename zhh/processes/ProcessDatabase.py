import sqlite3 as sqlite
from glob import glob
import os.path as osp
import os

class ProcessDatabase():
    def __init__(self, db_path, root_path:str, force_recreate:bool=False):
        self.db_path = db_path
        self.root_path = root_path
        
        if force_recreate:
            os.remove(db_path)
        
        self.conn = sqlite.connect(db_path)
        self.cursor = self.conn.cursor()

    def __del__(self):
        self.conn.close()

    def create_table(self, table_name, columns):
        columns_str = ', '.join(columns)
        self.cursor.execute(f"CREATE TABLE {table_name} ({columns_str})")
        self.conn.commit()

    def insert(self, table_name, columns, values):
        columns_str = ', '.join(columns)
        values_str = ', '.join([f"'{value}'" for value in values])
        self.cursor.execute(f"INSERT INTO {table_name} ({columns_str}) VALUES ({values_str})")
        self.conn.commit()

    def select(self, table_name, columns, conditions=None):
        columns_str = ', '.join(columns)
        if conditions:
            conditions_str = ' AND '.join(conditions)
            self.cursor.execute(f"SELECT {columns_str} FROM {table_name} WHERE {conditions_str}")
        else:
            self.cursor.execute(f"SELECT {columns_str} FROM {table_name}")
        return self.cursor.fetchall()

    def update(self, table_name, columns, values, conditions):
        columns_str = ', '.join([f"{column}='{value}'" for column, value in zip(columns, values)])
        conditions_str = ' AND '.join(conditions)
        self.cursor.execute(f"UPDATE {table_name} SET {columns_str} WHERE {conditions_str}")
        self.conn.commit()

    def delete(self, table_name, conditions):
        conditions_str = ' AND '.join(conditions)
        self.cursor.execute(f"DELETE FROM {table_name} WHERE {conditions_str}")
        self.conn.commit()