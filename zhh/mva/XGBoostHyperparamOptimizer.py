from io import StringIO
from xgboost import XGBClassifier
from optuna.storages import JournalStorage
from optuna.storages.journal import JournalFileBackend
from multiprocessing import Pool, cpu_count
from datetime import datetime
from math import sqrt, ceil
from collections.abs import Callable
import os, pickle, sys, argparse
import optuna
import numpy as np

class SklearnHyperparamOptimizer:
    def __init__(self, spawn_model:Callable, ):
        self._spawn = spawn_model
        
        
        pass