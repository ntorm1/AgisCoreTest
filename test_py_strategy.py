import os 
import sys

os.add_dll_directory('C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\x64\\Release')
os.add_dll_directory('C:\\Program Files (x86)\\Intel\\oneAPI\\tbb\\2021.9.0\\redist\\intel64\\vc_mt')
sys.path.append("C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\x64\\Release")


import AgisCorePy


class DummyStrategy(AgisCorePy.AgisStrategy):

	def __init__(self, strategy_id: str, portfolio, allocation: float):
		super().__init__(strategy_id, portfolio, allocation)
		self.portfolio = portfolio
		self.strategy_id = strategy_id

	def next(self):
		view = self.exchange.get_exchange_view("CLOSE")
		view.apply_weights("UNIFORM", 1)
		self.strategy_allocate(
			view, 
			0.0,
			alloc_type = AgisCorePy.AllocType.PCT
		)

	def reset(self):
		return

	def build(self):
		self.exchange = self.get_exchange("exchange1")
		self.exchange_subscribe("exchange1")
